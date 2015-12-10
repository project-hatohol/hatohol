#!/bin/sh

BASE_DIR="`dirname $0`"
TOP_DIR="$BASE_DIR/.."

TARGETS_OPT=""
if [ $# -ge 1 ]; then
  echo "target: " $1
  TARGETS_OPT="-t /$1/"
fi

${TOP_DIR}/server/hap2/hatohol/test/setup.sh
${TOP_DIR}/server/test/run-test.sh ${TARGETS_OPT}
