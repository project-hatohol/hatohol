#!/bin/sh

. $(dirname $0)/hap2-control-functions.sh
PLUGIN_PATH="${PLUGIN_DIR}/hatohol/hap2_fluentd.py"
run $@
