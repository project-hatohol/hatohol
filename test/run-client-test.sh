#!/bin/sh

FAILED=0
BASE_DIR="`dirname $0`"
TOP_DIR="$BASE_DIR/.."

${TOP_DIR}/client/test/python/run-test.sh || FAILED=1
${TOP_DIR}/test/launch-hatohol-for-test.sh || exit 1
$(npm bin)/mocha-phantomjs http://localhost:8000/test/index.html || FAILED=1
exit $FAILED
