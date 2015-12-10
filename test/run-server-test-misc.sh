#!/bin/sh

FAILED=0
BASE_DIR="`dirname $0`"
TOP_DIR="$BASE_DIR/.."

sudo ${TOP_DIR}/server/hap2/hatohol/test/setup.sh || FAILED=1

${TOP_DIR}/server/mlpl/test/run-test.sh || FAILED=1
${TOP_DIR}/server/tools/test/run-test.sh || FAILED=1
${TOP_DIR}/server/hap2/hatohol/test/run-test.sh || FAILED=1
exit $FAILED
