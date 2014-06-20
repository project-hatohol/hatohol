#!/bin/sh

while :
do
  MLPL_LOGGER_LEVEL=ERR HATOHOL_DB_DIR=/dev/shm HATOHOL_MYSQL_ENGINE_MEMORY=1 ./run-test.sh "$@"
  if [ $? -ne 0 ]; then
    exit 1
  fi
  rm -f core*
done
