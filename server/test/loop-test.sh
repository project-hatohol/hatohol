#!/bin/sh

option=$1

while :
do
  MLPL_LOGGER_LEVEL=ERR HATOHOL_DB_DIR=/dev/shm HATOHOL_MYSQL_ENGINE_MEMORY=1 ./run-test.sh $option
  if [ $? -ne 0 ]; then
    exit 1
  fi
done
