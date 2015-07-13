#!/bin/sh

. $(dirname $0)/hap2-control-functions.sh
PLUGIN_PATH="${PLUGIN_DIR}/hatohol/hap2_zabbix_api.py"
run $@
