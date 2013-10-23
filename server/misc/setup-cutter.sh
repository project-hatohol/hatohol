#!/bin/sh

NUM_RETRY=10
n=0
exit_code=0
while [ $n -lt $NUM_RETRY ]
do
  curl https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh
  exit_code=$?
  if [ $exit_code -eq 0 ]; then
    break
  fi
  n=`expr $n + 1`
done
exit $exit_code
