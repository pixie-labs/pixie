// This file is adapted from README.md of https://github.com/jmoiron/sqlx.
package main

import (
	"fmt"
	"log"
	"strings"
	"time"

	"github.com/jmoiron/sqlx"
	_ "github.com/lib/pq"
	"github.com/spf13/pflag"
)

var schema = `CREATE TABLE IF NOT EXISTS person (
    first_name text,
    last_name text,
    email text
)`

type person struct {
	FirstName string `db:"first_name"`
	LastName  string `db:"last_name"`
	Email     string
}

func main() {
	address := pflag.String("address", "localhost:5432", "Postgres server hostname.")
	user := pflag.String("user", "postgres", "Postgres server username.")
	password := pflag.String("password", "postgres", "Postgres server password.")
	count := pflag.Int("count", 1, "The count of requests to make.")

	pflag.Parse()

	hostPort := strings.Split(*address, ":")
	if len(hostPort) != 2 {
		log.Fatalf("--address is ill-formed, got %s", *address)
	}

	psqlInfo := fmt.Sprintf("host=%s port=%s user=%s password=%s sslmode=disable connect_timeout=5",
		hostPort[0], hostPort[1], *user, *password)

	log.Printf("Connecting to postgres: %s", psqlInfo)

	db, err := sqlx.Connect("postgres", psqlInfo)
	if err != nil {
		log.Fatalln(err)
	}
	defer db.Close()

	for i := 0; i < *count; i++ {
		// exec the schema or fail; multi-statement Exec behavior varies between
		// database drivers;  pq will exec them all, sqlite3 won't, ymmv
		db.MustExec(schema)

		tx := db.MustBegin()
		tx.MustExec("INSERT INTO person (first_name, last_name, email) VALUES ($1, $2, $3)", "Jason", "Moiron", "jmoiron@jmoiron.net")
		// Named queries can use structs, so if you have an existing struct (i.e. person := &person{}) that you have populated, you can pass it in as &person
		tx.Commit()

		// You can also get a single result, a la QueryRow
		jason := person{}
		db.Get(&jason, "SELECT * FROM person WHERE first_name=$1", "Jason")
		fmt.Printf("%#v\n", jason)
		time.Sleep(1 * time.Second)
	}
}