initscript_DATA = hatohol.centos \
	hap2-zabbix-api \
	hap2-nagios-ndoutils \
	hap2-nagios-livestatus \
	hap2-ceilometer \
	hap2-fluentd
initscriptdir = $(pkgdatadir)

hap2-zabbix-api:
	HAP2_PLUGIN="zabbix_api" CONF_NAME_SUFFIX="zabbix-api" \
	sh generate_hap2_init_file.sh > hap2-zabbix-api
	chmod +x hap2-zabbix-api
hap2-nagios-ndoutils:
	HAP2_PLUGIN="nagios_ndoutils" CONF_NAME_SUFFIX="nagios-ndoutils" \
	sh generate_hap2_init_file.sh > hap2-nagios-ndoutils
	chmod +x hap2-nagios-ndoutils
hap2-nagios-livestatus:
	HAP2_PLUGIN="nagios_livestatus" CONF_NAME_SUFFIX="nagios-livestatus" \
	sh generate_hap2_init_file.sh > hap2-nagios-livestatus
	chmod +x hap2-nagios-livestatus
hap2-ceilometer:
	HAP2_PLUGIN="ceilometer" CONF_NAME_SUFFIX="ceilometer" \
	sh generate_hap2_init_file.sh > hap2-ceilometer
	chmod +x hap2-ceilometer
hap2-fluentd:
	HAP2_PLUGIN="fluentd" CONF_NAME_SUFFIX="fluentd" \
	sh generate_hap2_init_file.sh > hap2-fluentd
	chmod +x hap2-fluentd
