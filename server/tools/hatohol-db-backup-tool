#!/usr/bin/env python
"""
  Copyright (C) 2014 Project Hatohol

  This file is part of Hatohol.

  Hatohol is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  Hatohol is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
"""
import sys
import subprocess
import shutil
import os

ConfigDBName = "hatohol"
ClientDBName = "hatohol_client"

def dumpConfigDB():
    cmd = []
    cmd.append("mysqldump")
    cmd.append("-u")
    cmd.append("hatohol")
    cmd.append("-phatohol")
    cmd.append(ConfigDBName)

    subprocess.Popen(cmd, stdout=open("./hatohol_backupdb/hatohol.mysql", "w"))

def dumpClientDB():
    cmd = []
    cmd.append("mysqldump")
    cmd.append("-u")
    cmd.append("hatohol")
    cmd.append("-phatohol")
    cmd.append(ClientDBName)

    subprocess.Popen(cmd, stdout=open("./hatohol_backupdb/hatohol_client.mysql", "w"))

def dumpCacheDB(dir):
    if (os.path.isdir(dir)):
        shutil.copytree(dir, "hatohol_backupdb/cache")
    else:
        print "There is no directory."

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
