#!/bin/sh

SQLITE3_PROGRAM="sqlite3"

if [ $# -lt 1 ]; then
	echo "Need one argument for DB name."
	return 1;
fi
DB_NAME=$1

${SQLITE3_PROGRAM} ${DB_NAME} "CREATE TABLE foo (id INTEGER PRIMARY KEY)"
${SQLITE3_PROGRAM} ${DB_NAME} "INSERT INTO foo VALUES(1)"
