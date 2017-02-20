configure_file(generate_service_file.sh.in generate_service_file.sh @ONLY)
add_custom_command(
	OUTPUT hatohol.service
	COMMAND sh generate_service_file.sh > hatohol.service
)

configure_file(generate_hap2_service_file.sh.in generate_hap2_service_file.sh @ONLY)
add_custom_command(
	OUTPUT hap2-zabbix-api.service
	COMMAND env "DESC=HAP2 for Zabbix API" "HAP2_PLUGIN=zabbix_api" "SUFFIX=zabbix-api"
	sh generate_hap2_service_file.sh > hap2-zabbix-api.service
)
add_custom_command(
	OUTPUT hap2-nagios-ndoutils.service
	COMMAND env "DESC=HAP2 for Nagios Ndoutils" "HAP2_PLUGIN=nagios_ndoutils" "SUFFIX=nagios-ndoutils"
	sh generate_hap2_service_file.sh > hap2-nagios-ndoutils.service
)
add_custom_command(
	OUTPUT hap2-nagios-livestatus.service
	COMMAND env "DESC=HAP2 for Nagios Livestatus" "HAP2_PLUGIN=nagios_livestatus" "SUFFIX=nagios-livestatus"
	sh generate_hap2_service_file.sh > hap2-nagios-livestatus.service
)
add_custom_command(
	OUTPUT hap2-ceilometer.service
	COMMAND env "DESC=HAP2 for Ceilometer" "HAP2_PLUGIN=ceilometer" "SUFFIX=ceilometer"
	sh generate_hap2_service_file.sh > hap2-ceilometer.service
)
add_custom_command(
	OUTPUT hap2-fluentd.service
	COMMAND env "DESC=HAP2 for Fluentd" "HAP2_PLUGIN=fluentd" "SUFFIX=fluentd"
	sh generate_hap2_service_file.sh > hap2-fluentd.service
)
add_custom_target(systemd ALL SOURCES
	hatohol.service
	hap2-zabbix-api.service
	hap2-nagios-ndoutils.service
	hap2-nagios-livestatus.service
	hap2-ceilometer.service
	hap2-fluentd.service
)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/hatohol.service
	${CMAKE_CURRENT_BINARY_DIR}/hap2-zabbix-api.service
	${CMAKE_CURRENT_BINARY_DIR}/hap2-nagios-ndoutils.service
	${CMAKE_CURRENT_BINARY_DIR}/hap2-nagios-livestatus.service
	${CMAKE_CURRENT_BINARY_DIR}/hap2-ceilometer.service
	${CMAKE_CURRENT_BINARY_DIR}/hap2-fluentd.service
	DESTINATION usr/lib/systemd/system
)

configure_file(hatohol.conf.in hatohol.conf @ONLY)
install(FILES hatohol.conf DESTINATION etc/hatohol)
install(FILES hatohol.conf DESTINATION etc/hatohol RENAME hap2-zabbix-api.conf)
install(FILES hatohol.conf DESTINATION etc/hatohol RENAME hap2-nagios-ndoutils.conf)
install(FILES hatohol.conf DESTINATION etc/hatohol RENAME hap2-nagios-livestatus.conf)
install(FILES hatohol.conf DESTINATION etc/hatohol RENAME hap2-fluentd.conf)
install(FILES hatohol.conf DESTINATION etc/hatohol RENAME hap2-ceilometer.conf)
