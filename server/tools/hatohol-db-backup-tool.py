#!/usr/bin/env python
import sys
import subprocess
import shutil
import os

def dumpConfigDB():
    cmd = []
    cmd.append("mysqldump")
    cmd.append("-u")
    cmd.append("hatohol")
    cmd.append("-phatohol")
    cmd.append("hatohol")

    subprocess.Popen(cmd, stdout=open("./hatohol_backupdb/hatohol.mysql", "w"))

def dumpClientDB():
    cmd = []
    cmd.append("mysqldump")
    cmd.append("-u")
    cmd.append("hatohol")
    cmd.append("-phatohol")
    cmd.append("hatohol_client")

    subprocess.Popen(cmd, stdout=open("./hatohol_backupdb/hatohol_client.mysql", "w"))

def dumpCacheDB(dir):
    shutil.copytree(dir, "hatohol_backupdb/cache")

def createTmpDirectory():
    os.mkdir("hatohol_backupdb")

def deleteTmpDirectory():
    shutil.rmtree("hatohol_backupdb")

def createTarball():
    cmd = []
    cmd.append("tar")
    cmd.append("zcvf")
    cmd.append("hatohol_backupdb.tar.gz")
    cmd.append("hatohol_backupdb")

    subprocess.call(cmd)
    deleteTmpDirectory()

def showUsage():
    print "Usage:\n"
    print "$ %s <Path of CacheDB>" % sys.argv[0]


# ---------------
# Main routine
# ---------------
if (len(sys.argv) < 2):
    showUsage()
    quit()

createTmpDirectory()
dumpConfigDB()
dumpClientDB()
dumpCacheDB(sys.argv[1])
createTarball()
