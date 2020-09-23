package main

import (
	"net/http"

	"pixielabs.ai/pixielabs/src/shared/services/env"

	"github.com/golang-migrate/migrate"
	"github.com/golang-migrate/migrate/database/postgres"
	bindata "github.com/golang-migrate/migrate/source/go_bindata"
	log "github.com/sirupsen/logrus"
	controllers "pixielabs.ai/pixielabs/src/cloud/project_manager/controller"
	"pixielabs.ai/pixielabs/src/cloud/project_manager/datastore"
	"pixielabs.ai/pixielabs/src/cloud/project_manager/projectmanagerpb"
	"pixielabs.ai/pixielabs/src/cloud/project_manager/schema"
	"pixielabs.ai/pixielabs/src/shared/services"
	"pixielabs.ai/pixielabs/src/shared/services/healthz"
	"pixielabs.ai/pixielabs/src/shared/services/pg"
)

func main() {
	log.WithField("service", "project-manager-service").Info("Starting service")

	services.SetupService("project-manager-service", 50300)
	services.PostFlagSetupAndParse()
	services.CheckServiceFlags()
	services.SetupServiceLogging()

	mux := http.NewServeMux()
	healthz.RegisterDefaultChecks(mux)

	db := pg.MustConnectDefaultPostgresDB()

	// TODO(zasgar): Pull out this migration code into a util. Just leaving it here for now for testing.
	driver, err := postgres.WithInstance(db.DB, &postgres.Config{})

	sc := bindata.Resource(schema.AssetNames(), func(name string) (bytes []byte, e error) {
		return schema.Asset(name)
	})

	d, err := bindata.WithInstance(sc)

	mg, err := migrate.NewWithInstance(
		"go-bindata",
		d, "postgres", driver)

	if err = mg.Up(); err != nil {
		log.WithError(err).Info("migrations failed: %s", err)
	}

	datastore, err := datastore.NewDatastore(db)
	if err != nil {
		log.WithError(err).Fatalf("Failed to initialize datastore")
	}

	server := controllers.NewServer(datastore)
	s := services.NewPLServer(env.New(), mux)
	projectmanagerpb.RegisterProjectManagerServiceServer(s.GRPCServer(), server)

	s.Start()
	s.StopOnInterrupt()
}