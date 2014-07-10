#!/bin/sh

DIR=$1

dumpConfigDB() {
    mysqldump -u hatohol -phatohol hatohol > ./hatohol.mysql
    mysqldump -u hatohol -phatohol hatohol_client > ./hatohol_client.mysql
}

copyCacheDB() {
    if [ -e $DIR ]; then
        cp $DIR/hatohol*.db ./
    else
        echo "There is no directory"
    fi
}

showUsage() {
    echo "Usage:"
    echo "hatohol-db-backup-tool.sh <Path of CacheDB>"
}

if [ $# -ne 1 ]; then
    showUsage
else
    dumpConfigDB
    copyCacheDB
fi
