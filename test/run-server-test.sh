#!/bin/sh

FAILED=0
BASE_DIR="`dirname $0`"
TOP_DIR="$BASE_DIR/.."

${TOP_DIR}/server/mlpl/test/run-test.sh || FAILED=1
${TOP_DIR}/server/test/run-test.sh || FAILED=1
${TOP_DIR}/server/tools/test/run-test.sh || FAILED=1
sudo ${TOP_DIR}/server/hap2/test/setup.sh || FAILED=1
${TOP_DIR}/server/hap2/hatohol/test/run-test.sh || FAILED=1
exit $FAILED
