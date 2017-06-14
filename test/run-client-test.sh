#!/bin/sh

FAILED=0
BASE_DIR="`dirname $0`"
TOP_DIR="$BASE_DIR/.."

${TOP_DIR}/client/test/python/run-test.sh || FAILED=1

export PIDS_FILE=`pwd`/pids_file
rm -fr $PIDS_FILE
${TOP_DIR}/test/launch-hatohol-for-test.sh || exit 1
$(npm bin)/mocha-phantomjs http://localhost:8000/test/index.html || FAILED=1
# manage.py has a child process. We have to kill it too.
cat $PIDS_FILE | awk '{ print "ps h -p " $1 " --ppid " $1 " -o pid" }' | sh | awk '{ print "kill " $1 }' | sh
npm run lint || FAILED=1

exit $FAILED
