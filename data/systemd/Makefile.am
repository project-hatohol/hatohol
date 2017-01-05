initscript_DATA = \
	hatohol.service \
	hap2-zabbix-api.service \
	hap2-nagios-ndoutils.service \
	hap2-nagios-livestatus.service \
	hap2-ceilometer.service \
	hap2-fluentd.service \
	hatohol.conf \
	hap2-zabbix-api.conf \
	hap2-nagios-ndoutils.conf \
	hap2-nagios-livestatus.conf \
	hap2-ceilometer.conf \
	hap2-fluentd.conf

hatohol.service:
	sh generate_service_file.sh > hatohol.service
hap2-zabbix-api.service:
	DESC="HAP2 for Zabbix API" HAP2_PLUGIN="zabbix_api" SUFFIX="zabbix-api" \
	sh generate_hap2_service_file.sh > hap2-zabbix-api.service
hap2-nagios-ndoutils.service:
	DESC="HAP2 for Nagios Ndoutils" HAP2_PLUGIN="nagios_ndoutils" SUFFIX="nagios-ndoutils" \
	sh generate_hap2_service_file.sh > hap2-nagios-ndoutils.service
hap2-nagios-livestatus.service:
	DESC="HAP2 for Nagios Livestatus" HAP2_PLUGIN="nagios_livestatus" SUFFIX="nagios-livestatus" \
	sh generate_hap2_service_file.sh > hap2-nagios-livestatus.service
hap2-ceilometer.service:
	DESC="HAP2 for Ceilometer" HAP2_PLUGIN="ceilometer" SUFFIX="ceilometer" \
	sh generate_hap2_service_file.sh > hap2-ceilometer.service
hap2-fluentd.service:
	DESC="HAP2 for Fluentd" HAP2_PLUGIN="fluentd" SUFFIX="fluentd" \
	sh generate_hap2_service_file.sh > hap2-fluentd.service
hap2-zabbix-api.conf:
	cp hatohol.conf hap2-zabbix-api.conf
hap2-nagios-ndoutils.conf:
	cp hatohol.conf hap2-nagios-ndoutils.conf
hap2-nagios-livestatus.conf:
	cp hatohol.conf hap2-nagios-livestatus.conf
hap2-fluentd.conf:
	cp hatohol.conf hap2-fluentd.conf
hap2-ceilometer.conf:
	cp hatohol.conf hap2-ceilometer.conf

initscriptdir = $(pkgdatadir)
EXTRA_DIST = \
	hatohol.conf \
	hap2-zabbix-api.conf \
	hap2-nagios-ndoutils.conf \
	hap2-nagios-livestatus.conf \
	hap2-fluentd.conf \
	hap2-ceilometer.conf
