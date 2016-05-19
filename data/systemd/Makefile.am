initscript_DATA = \
	hatohol.service \
	hatohol.conf \
	hap2-zabbix-api.service \
	hap2-nagios-ndoutils.service \
	hap2-nagios-livestatus.service \
	hap2-ceilometer.service \
	hap2-fluentd.service

hatohol.service:
	sh generate_service_file.sh > hatohol.service
hap2-zabbix-api.service:
	DESC="HAP2 for Zabbix API" HAP2_PLUGIN="zabbix_api" CONF_NAME_SUFFIX="zabbix-api" \
	sh generate_hap2_service_file.sh > hap2-zabbix-api.service
hap2-nagios-ndoutils.service:
	DESC="HAP2 for Nagios Ndoutils" HAP2_PLUGIN="nagios_ndoutils" CONF_NAME_SUFFIX="nagios-ndoutils" \
	sh generate_hap2_service_file.sh > hap2-nagios-ndoutils.service
hap2-nagios-livestatus.service:
	DESC="HAP2 for Nagios Livestatus" HAP2_PLUGIN="nagios_livestatus" CONF_NAME_SUFFIX="nagios-livestatus" \
	sh generate_hap2_service_file.sh > hap2-nagios-livestatus.service
hap2-ceilometer.service:
	DESC="HAP2 for Ceilometer" HAP2_PLUGIN="ceilometer" CONF_NAME_SUFFIX="ceilometer" \
	sh generate_hap2_service_file.sh > hap2-ceilometer.service
hap2-fluentd.service:
	DESC="HAP2 for Fluentd" HAP2_PLUGIN="fluentd" CONF_NAME_SUFFIX="fluentd" \
	sh generate_hap2_service_file.sh > hap2-fluentd.service

initscriptdir = $(pkgdatadir)
EXTRA_DIST = hatohol.conf
