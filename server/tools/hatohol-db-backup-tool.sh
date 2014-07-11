#!/bin/sh

DIR=$1

dumpConfigDB() {
    mysqldump -u hatohol -phatohol hatohol > ./hatohol_backupdb/hatohol.mysql
    mysqldump -u hatohol -phatohol hatohol_client > ./hatohol_backupdb/hatohol_client.mysql
}

copyCacheDB() {
    if [ -e $DIR ]; then
        cp $DIR/hatohol*.db ./hatohol_backupdb/
    else
        echo "There is no directory"
    fi
}

showUsage() {
    echo "Usage:"
    echo "hatohol-db-backup-tool.sh <Path of CacheDB>"
}

createTarball() {
    tar zcvf hatohol_backupdb.tar.gz hatohol_backupdb/
    rm -rf hatohol_backupdb/
}

if [ $# -ne 1 ]; then
    showUsage
else
    mkdir hatohol_backupdb
    dumpConfigDB
    copyCacheDB
    createTarball
fi
