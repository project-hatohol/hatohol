initscript_DATA = \
	hatohol.service \
	hatohol.conf \
	hap2_zabbix_api.service \
	hap2_nagios_ndoutils.service \
	hap2_nagios_livestatus.service \
	hap2_ceilometer.service \
	hap2_fluentd.service

hatohol.service:
	sh generate_service_file.sh > hatohol.service
hap2_zabbix_api.service:
	DESC="HAP2 for Zabbix API" HAP2_PLUGIN="zabbix_api" CONF_NAME_SUFFIX="zabbix-api" \
	sh generate_hap2_service_file.sh > hap2_zabbix_api.service
hap2_nagios_ndoutils.service:
	DESC="HAP2 for Nagios Ndoutils" HAP2_PLUGIN="nagios_ndoutils" CONF_NAME_SUFFIX="nagios-ndoutils" \
	sh generate_hap2_service_file.sh > hap2_nagios_ndoutils.service
hap2_nagios_livestatus.service:
	DESC="HAP2 for Nagios Livestatus" HAP2_PLUGIN="nagios_livestatus" CONF_NAME_SUFFIX="nagios-livestatus" \
	sh generate_hap2_service_file.sh > hap2_nagios_livestatus.service
hap2_ceilometer.service:
	DESC="HAP2 for Ceilometer" HAP2_PLUGIN="ceilometer" CONF_NAME_SUFFIX="ceilometer" \
	sh generate_hap2_service_file.sh > hap2_ceilometer.service
hap2_fluentd.service:
	DESC="HAP2 for Fluentd" HAP2_PLUGIN="fluentd" CONF_NAME_SUFFIX="fluentd" \
	sh generate_hap2_service_file.sh > hap2_fluentd.service

initscriptdir = $(pkgdatadir)
EXTRA_DIST = hatohol.conf
