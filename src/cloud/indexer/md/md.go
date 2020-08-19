package md

import (
	"context"
	"fmt"

	"github.com/nats-io/stan.go"
	"github.com/olivere/elastic/v7"
	uuid "github.com/satori/go.uuid"
	log "github.com/sirupsen/logrus"

	mdpb "pixielabs.ai/pixielabs/src/shared/k8s/metadatapb"
)

// VizierIndexer run the indexer for a single vizier index.
type VizierIndexer struct {
	sc       stan.Conn
	es       *elastic.Client
	vizierID uuid.UUID
	orgID    uuid.UUID
	k8sUID   string

	sub    stan.Subscription
	quitCh chan bool
	errCh  chan error
}

// NewVizierIndexer creates a new Vizier indexer.
func NewVizierIndexer(vizierID uuid.UUID, orgID uuid.UUID, k8sUID string, sc stan.Conn, es *elastic.Client) *VizierIndexer {
	return &VizierIndexer{
		sc:       sc,
		es:       es,
		vizierID: vizierID,
		orgID:    orgID,
		k8sUID:   k8sUID,
		quitCh:   make(chan bool),
		errCh:    make(chan error),
	}
}

// Run starts the indexer.
func (v *VizierIndexer) Run(topic string) error {
	log.
		WithField("VizierID", v.vizierID).
		WithField("ClusterUID", v.k8sUID).
		Info("Starting Indexer")

	sub, err := v.sc.Subscribe(topic, v.stanMessageHandler, stan.DurableName("indexer"), stan.SetManualAckMode(), stan.DeliverAllAvailable())
	if err != nil {
		log.WithError(err).Error("Failed to subscribe")
	}
	v.sub = sub

	for {
		select {
		case <-v.quitCh:
			return nil
		case <-v.errCh:
			log.WithField("vizier", v.vizierID.String()).WithError(err).Error("Error during indexing")
		}
	}
}

// Stop stops the indexer.
func (v *VizierIndexer) Stop() {
	close(v.quitCh)
	err := v.sub.Unsubscribe()
	if err != nil {
		log.WithError(err).Error("Failed to un-subscribe from channel")
	}
}

func (v *VizierIndexer) nsUpdateToEMD(u *mdpb.ResourceUpdate, nsUpdate *mdpb.NamespaceUpdate) *EsMDEntity {
	return &EsMDEntity{
		OrgID:              v.orgID.String(),
		VizierID:           v.vizierID.String(),
		ClusterUID:         v.k8sUID,
		UID:                nsUpdate.UID,
		Name:               nsUpdate.Name,
		NS:                 nsUpdate.Name,
		Kind:               "namespace",
		TimeStartedNS:      nsUpdate.StartTimestampNS,
		TimeStoppedNS:      nsUpdate.StopTimestampNS,
		RelatedEntityNames: []string{},
		ResourceVersion:    u.ResourceVersion,
	}
}
func (v *VizierIndexer) podUpdateToEMD(u *mdpb.ResourceUpdate, podUpdate *mdpb.PodUpdate) *EsMDEntity {
	return &EsMDEntity{
		OrgID:              v.orgID.String(),
		VizierID:           v.vizierID.String(),
		ClusterUID:         v.k8sUID,
		UID:                podUpdate.UID,
		Name:               podUpdate.Name,
		NS:                 podUpdate.Namespace,
		Kind:               "pod",
		TimeStartedNS:      podUpdate.StartTimestampNS,
		TimeStoppedNS:      podUpdate.StopTimestampNS,
		RelatedEntityNames: []string{},
		ResourceVersion:    u.ResourceVersion,
	}
}

func (v *VizierIndexer) serviceUpdateToEMD(u *mdpb.ResourceUpdate, serviceUpdate *mdpb.ServiceUpdate) *EsMDEntity {
	if serviceUpdate.PodIDs == nil {
		serviceUpdate.PodIDs = make([]string, 0)
	}
	return &EsMDEntity{
		OrgID:         v.orgID.String(),
		VizierID:      v.vizierID.String(),
		ClusterUID:    v.k8sUID,
		UID:           serviceUpdate.UID,
		Name:          serviceUpdate.Name,
		NS:            serviceUpdate.Namespace,
		Kind:          "service",
		TimeStartedNS: serviceUpdate.StartTimestampNS,
		TimeStoppedNS: serviceUpdate.StopTimestampNS,
		RelatedEntityNames: serviceUpdate.PodIDs,
		ResourceVersion:    u.ResourceVersion,
	}
}

func (v *VizierIndexer) resourceUpdateToEMD(update *mdpb.ResourceUpdate) *EsMDEntity {
	switch update.Update.(type) {
	case *mdpb.ResourceUpdate_NamespaceUpdate:
		return v.nsUpdateToEMD(update, update.GetNamespaceUpdate())
	case *mdpb.ResourceUpdate_PodUpdate:
		return v.podUpdateToEMD(update, update.GetPodUpdate())
	case *mdpb.ResourceUpdate_ServiceUpdate:
		return v.serviceUpdateToEMD(update, update.GetServiceUpdate())
	default:
		log.Errorf("Unknown entity types: %v", update)
		return nil
	}
}

const elasticUpdateScript = `
if (params.resourceVersion.compareTo(ctx._source.resourceVersion) <= 0)  {
  ctx.op = 'noop';
}
ctx._source.relatedEntityNames.addAll(params.entities);
ctx._source.relatedEntityNames = ctx._source.relatedEntityNames.stream().sorted().collect(Collectors.toList());
ctx._source.timeStoppedNS = params.timeStoppedNS;
ctx._source.resourceVersion = params.resourceVersion;
`

func (v *VizierIndexer) stanMessageHandler(msg *stan.Msg) {
	ru := mdpb.ResourceUpdate{}
	err := ru.Unmarshal(msg.Data)
	if err != nil { // We received an invalid message through stan.
		log.WithError(err).Error("Could not unmarshal message from stan")
		v.errCh <- err
		msg.Ack()
		return
	}

	err = v.HandleResourceUpdate(&ru)
	if err != nil {
		log.WithError(err).Error("Error handling resource update")
		v.errCh <- err
		msg.Ack()
		return
	}

	msg.Ack()
}

// HandleResourceUpdate indexes the resource update in elastic.
func (v *VizierIndexer) HandleResourceUpdate(update *mdpb.ResourceUpdate) error {
	esEntity := v.resourceUpdateToEMD(update)
	if esEntity == nil { // We are not handling this resource yet.
		return nil
	}

	id := fmt.Sprintf("%s-%s-%s", v.vizierID, v.k8sUID, esEntity.UID)
	_, err := v.es.Update().
		Index(indexName).
		Id(id).
		Script(
			elastic.NewScript(elasticUpdateScript).
				Param("entities", esEntity.RelatedEntityNames).
				Param("timeStoppedNS", esEntity.TimeStoppedNS).
				Param("resourceVersion", esEntity.ResourceVersion).
				Lang("painless")).
		Upsert(esEntity).
		Refresh("true").
		Do(context.Background())
	return err
}