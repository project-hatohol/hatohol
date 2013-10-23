#!/bin/sh
pkill -9 hatohol
ps ax | grep "python ./manage.py" | grep -v grep | awk '{ print "kill -9 " $1 }' | sh

