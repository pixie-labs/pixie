package controller

import (
	"errors"
	"fmt"
	"sync"
	"time"

	"github.com/gogo/protobuf/proto"
	"github.com/gogo/protobuf/types"
	"github.com/jmoiron/sqlx"
	"github.com/nats-io/nats.go"
	"github.com/nats-io/stan.go"
	uuid "github.com/satori/go.uuid"
	log "github.com/sirupsen/logrus"

	"pixielabs.ai/pixielabs/src/cloud/shared/messages"
	messagespb "pixielabs.ai/pixielabs/src/cloud/shared/messagespb"
	"pixielabs.ai/pixielabs/src/cloud/shared/vzshard"
	"pixielabs.ai/pixielabs/src/shared/cvmsgspb"
	metadatapb "pixielabs.ai/pixielabs/src/shared/k8s/metadatapb"
	"pixielabs.ai/pixielabs/src/utils"
)

// The topic on which to make metadata requests.
const metadataRequestTopic = "MetadataRequest"

// The topic on which to listen to metadata responses.
const metadataResponseTopic = "MetadataResponse"

// The topic on which to listen for streaming metadata updates.
const streamingMetadataTopic = "DurableMetadataUpdates"

// The topic on which to write updates to.
const indexerMetadataTopic = "MetadataIndex"

// VizierState contains all state necessary to process metadata updates for the given vizier.
type VizierState struct {
	id              uuid.UUID // The Vizier's ID.
	resourceVersion string    // The Vizier's last applied resource version.

	liveSub stan.Subscription // The subcription for the live metadata updates.
	liveCh  chan *stan.Msg

	quitCh chan bool // Channel to sign a stop for a particular vizier
}

// MetadataReader reads updates from the NATS durable queue and sends updates to the indexer.
type MetadataReader struct {
	db *sqlx.DB

	sc stan.Conn
	nc *nats.Conn

	viziers map[uuid.UUID]*VizierState // Map of Vizier ID to its state.
	mu      sync.Mutex                 // Mutex for viziers map.

	quitCh chan bool // Channel to signal a stop for all viziers
}

// NewMetadataReader creates a new MetadataReader.
func NewMetadataReader(db *sqlx.DB, sc stan.Conn, nc *nats.Conn) (*MetadataReader, error) {
	viziers := make(map[uuid.UUID]*VizierState)

	m := &MetadataReader{db: db, sc: sc, nc: nc, viziers: viziers}
	err := m.loadState()
	if err != nil {
		return nil, err
	}

	return m, nil
}

// listenForViziers listens for any newly connected Viziers and subscribes to their update channel.
func (m *MetadataReader) listenForViziers() {
	ch := make(chan *nats.Msg)
	sub, err := m.nc.ChanSubscribe(messages.VizierConnectedChannel, ch)
	if err != nil {
		log.WithError(err).Error("Failed to listen for connected viziers")
		return
	}

	defer sub.Unsubscribe()
	defer close(ch)
	for {
		select {
		case <-m.quitCh:
			log.Info("Quit signaled")
			return
		case msg := <-ch:
			vcMsg := &messagespb.VizierConnected{}
			err := proto.Unmarshal(msg.Data, vcMsg)
			if err != nil {
				log.WithError(err).Error("Could not unmarshal VizierConnected msg")
			}
			vzID := utils.UUIDFromProtoOrNil(vcMsg.VizierID)
			log.WithField("VizierID", vzID.String()).Info("Listening to metadata updates for Vizier")

			err = m.StartVizierUpdates(vzID, vcMsg.ResourceVersion)
			if err != nil {
				log.WithError(err).WithField("VizierID", vzID.String()).Error("Could not start listening to updates from Vizier")
			}
		}
	}
}

func (m *MetadataReader) loadState() error {
	// Start listening for any newly connected Viziers.
	go m.listenForViziers()

	// Start listening to updates for any Viziers that are already connected to Cloud.
	err := m.listenToConnectedViziers()
	if err != nil {
		log.WithError(err).Info("Failed to load state")
		return err
	}

	return nil
}

func (m *MetadataReader) listenToConnectedViziers() error {
	query := `SELECT vizier_cluster_id, resource_version from vizier_cluster_info AS c INNER JOIN vizier_index_state AS i ON c.vizier_cluster_id = i.cluster_id WHERE c.status != 'UNKNOWN' AND c.status != 'DISCONNECTED'`
	var val struct {
		ID              uuid.UUID `db:"vizier_cluster_id"`
		ResourceVersion string    `db:"resource_version"`
	}

	rows, err := m.db.Queryx(query)
	if err != nil {
		return err
	}
	defer rows.Close()
	for rows.Next() {
		err := rows.StructScan(&val)
		if err != nil {
			return err
		}
		err = m.StartVizierUpdates(val.ID, val.ResourceVersion)
		if err != nil {
			return err
		}
	}

	return nil
}

// StartVizierUpdates starts listening to the metadata update channel for a given vizier.
func (m *MetadataReader) StartVizierUpdates(id uuid.UUID, rv string) error {
	// TODO(michelle): We currently don't have to signal when a Vizier has disconnected. When we have that
	// functionality, we should clean up the Vizier map and stop its STAN subscriptions.

	m.mu.Lock()
	defer m.mu.Unlock()

	if _, ok := m.viziers[id]; ok {
		log.WithField("vizier_id", id.String()).Info("Already listening to metadata updates from Vizier")
		return nil
	}

	vzState := VizierState{
		id:              id,
		resourceVersion: rv,
		liveCh:          make(chan *stan.Msg),
		quitCh:          make(chan bool),
	}

	// Subscribe to STAN topic for streaming updates.
	topic := vzshard.V2CTopic(streamingMetadataTopic, id)
	log.WithField("topic", topic).Info("Subscribing to STAN")
	liveSub, err := m.sc.Subscribe(topic, func(msg *stan.Msg) {
		vzState.liveCh <- msg
	}, stan.SetManualAckMode())
	if err != nil {
		close(vzState.liveCh)
		close(vzState.quitCh)
		return err
	}

	vzState.liveSub = liveSub

	m.viziers[id] = &vzState

	go m.ProcessVizierUpdates(&vzState)

	return nil
}

// StopVizierUpdates stops listening to the metadata update channel for a given vizier.
func (m *MetadataReader) StopVizierUpdates(id uuid.UUID) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if vz, ok := m.viziers[id]; ok {
		vz.liveSub.Unsubscribe()
		close(vz.liveCh)
		close(vz.quitCh)

		delete(m.viziers, id)
		return nil
	}

	return errors.New("Vizier doesn't exist")
}

// ProcessVizierUpdates reads from the metadata updates and sends them to the indexer in order.
func (m *MetadataReader) ProcessVizierUpdates(vzState *VizierState) error {
	// Clean up if any error has occurred.
	defer m.StopVizierUpdates(vzState.id)

	for {
		select {
		case <-m.quitCh:
			return nil
		case <-vzState.quitCh:
			return nil
		case msg := <-vzState.liveCh:
			err := m.processVizierUpdate(msg, vzState)
			if err != nil {
				log.WithError(err).Error("Error processing Vizier metadata updates")
				return err
			}
		}
	}
}

func readMetadataUpdate(data []byte) (*cvmsgspb.MetadataUpdate, error) {
	v2cMsg := &cvmsgspb.V2CMessage{}
	err := proto.Unmarshal(data, v2cMsg)
	if err != nil {
		return nil, err
	}
	updateMsg := &cvmsgspb.MetadataUpdate{}
	err = types.UnmarshalAny(v2cMsg.Msg, updateMsg)
	if err != nil {
		log.WithError(err).Error("Could not unmarshal metadata update message")
		return nil, err
	}
	return updateMsg, nil
}

func readMetadataResponse(data []byte) (*cvmsgspb.MetadataResponse, error) {
	v2cMsg := &cvmsgspb.V2CMessage{}
	err := proto.Unmarshal(data, v2cMsg)
	if err != nil {
		return nil, err
	}
	reqUpdates := &cvmsgspb.MetadataResponse{}
	err = types.UnmarshalAny(v2cMsg.Msg, reqUpdates)
	if err != nil {
		log.WithError(err).Error("Could not unmarshal metadata response message")
		return nil, err
	}
	return reqUpdates, nil
}

func wrapMetadataRequest(vizierID uuid.UUID, req *cvmsgspb.MetadataRequest) ([]byte, error) {
	reqAnyMsg, err := types.MarshalAny(req)
	if err != nil {
		return nil, err
	}
	c2vMsg := cvmsgspb.C2VMessage{
		VizierID: vizierID.String(),
		Msg:      reqAnyMsg,
	}
	b, err := c2vMsg.Marshal()
	if err != nil {
		return nil, err
	}
	return b, nil
}

func (m *MetadataReader) processVizierUpdate(msg *stan.Msg, vzState *VizierState) error {
	if msg == nil {
		return nil
	}

	updateMsg, err := readMetadataUpdate(msg.Data)
	if err != nil {
		return err
	}
	update := updateMsg.Update

	for update.PrevResourceVersion > vzState.resourceVersion {
		err = m.getMissingUpdates(vzState.resourceVersion, update.ResourceVersion, update.PrevResourceVersion, vzState)
		if err != nil {
			return err
		}
	}

	if update.PrevResourceVersion == vzState.resourceVersion {
		err := m.applyMetadataUpdates(vzState, []*metadatapb.ResourceUpdate{update})
		if err != nil {
			return err
		}
	}

	// It's possible that we've missed the message's 30-second ACK timeline, if it took a while to receive
	// missing metadata updates. This message will be sent back on STAN for reprocessing, but we will
	// ignore it since the ResourceVersion will be less than the current resource
	msg.Ack()
	return nil
}

func (m *MetadataReader) getMissingUpdates(from string, to string, expectedRV string, vzState *VizierState) error {
	log.WithField("vizier", vzState.id.String()).WithField("from", from).WithField("to", to).WithField("expected", expectedRV).Info("Making request for missing metadata updates")
	topicID := uuid.NewV4()
	topic := fmt.Sprintf("%s_%s", metadataResponseTopic, topicID.String())
	mdReq := &cvmsgspb.MetadataRequest{
		From:  from,
		To:    to,
		Topic: topic,
	}
	reqBytes, err := wrapMetadataRequest(vzState.id, mdReq)
	if err != nil {
		return err
	}

	// Subscribe to topic that the response will be sent on.
	subCh := make(chan *nats.Msg, 1024)
	sub, err := m.nc.ChanSubscribe(vzshard.V2CTopic(topic, vzState.id), subCh)
	defer sub.Unsubscribe()

	pubTopic := vzshard.C2VTopic(metadataRequestTopic, vzState.id)
	err = m.nc.Publish(pubTopic, reqBytes)
	if err != nil {
		return err
	}

	for {
		select {
		case <-m.quitCh:
			return errors.New("Quit signaled")
		case <-vzState.quitCh:
			return errors.New("Vizier removed")
		case msg := <-subCh:
			reqUpdates, err := readMetadataResponse(msg.Data)
			if err != nil {
				return err
			}

			updates := reqUpdates.Updates
			if len(updates) == 0 {
				return nil
			}
			if updates[0].PrevResourceVersion > vzState.resourceVersion {
				// Received out of order update. Need to rerequest metadata.
				log.WithField("prevRV", updates[0].PrevResourceVersion).WithField("RV", vzState.resourceVersion).Info("Received out of order update")
				return nil
			}

			err = m.applyMetadataUpdates(vzState, updates)
			if err != nil {
				return err
			}

			if vzState.resourceVersion >= expectedRV {
				return nil
			}
		case <-time.After(20 * time.Minute):
			// Our previous request shouldn't have gotten lost on NATS, so if there is a subscriber for the metadata
			// requests we shouldn't actually need to resend the request.
			return nil
		}
	}
}

func (m *MetadataReader) applyMetadataUpdates(vzState *VizierState, updates []*metadatapb.ResourceUpdate) error {
	for i, u := range updates {
		if u.ResourceVersion <= vzState.resourceVersion {
			continue // Don't send the update.
		}

		// Publish update to the indexer.
		b, err := updates[i].Marshal()
		if err != nil {
			return err
		}

		log.WithField("topic", indexerMetadataTopic).WithField("rv", u.ResourceVersion).Info("Publishing metadata update to indexer")

		err = m.sc.Publish(indexerMetadataTopic, b)
		if err != nil {
			return err
		}

		vzState.resourceVersion = u.ResourceVersion

		// Update index state in Postgres.
		query := `
	    UPDATE vizier_index_state
	    SET resource_version = $2
	    WHERE cluster_id = $1`
		row, err := m.db.Queryx(query, vzState.id, u.ResourceVersion)
		if err != nil {
			return err
		}
		row.Close()
	}
	return nil
}