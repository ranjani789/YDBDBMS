//////////////////////////////////////////////////////////////////
//								//
// Copyright (c) 2019 YottaDB LLC and/or its subsidiaries.	//
// All rights reserved.						//
//								//
//	This source code contains the intellectual property	//
//	of its copyright holder(s), and is made available	//
//	under a license.  If you do not know the terms of	//
//	the license, please stop and do not read further.	//
//								//
//////////////////////////////////////////////////////////////////

package main

import (
  "database/sql"
  _ "github.com/lib/pq"
  "fmt"
)

func main() {
  connStr := "host=127.0.0.1 port=1337 user=ydb password=ydbrocks dbname=hello sslmode=disable"
  db, err := sql.Open("postgres", connStr)
  if err != nil {
    panic(err)
  }

  rows, err := db.Query("SELECT * FROM names")
  if err != nil {
    panic(err)
  }

  rs, err := rows.ColumnTypes()
  if err != nil {
    panic(err)
  }
  for _, r := range rs {
    fmt.Printf("%v\n", r)
  }
}
