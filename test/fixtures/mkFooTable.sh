#!/bin/sh

SQLITE3="sqlite3"

if [ $# -lt 1 ]; then
	echo "Need one argument for DB name."
	return 1;
fi
DB_NAME=$1

${SQLITE3} ${DB_NAME} "CREATE TABLE foo (id INTEGER PRIMARY KEY)"
${SQLITE3} ${DB_NAME} "INSERT INTO foo VALUES(1)"
