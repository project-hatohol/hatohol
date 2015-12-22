sql_DATA = \
	create-db.sql \
	init-user.sql \
	server-type-zabbix.sql \
	server-type-nagios.sql \
	server-type-hap2-zabbix.sql \
	server-type-hap2-nagios.sql \
	server-type-hap2-ceilometer.sql \
	server-type-hap2-fluentd.sql \
	severity-ranks.sql \
	custom-incident-statuses.sql \
	incident-trackers.sql \
	actions.sql

if WITH_QPID
sql_DATA += \
	server-type-hapi-zabbix.sql \
	server-type-hapi-json.sql \
	server-type-ceilometer.sql
endif

sqldir = $(pkgdatadir)/sql

HATOHOL_SERVER_TYPE_UTIL = ../tools/hatohol-server-type-util
server-type: $(HATOHOL_SERVER_TYPE_UTIL)
	$(HATOHOL_SERVER_TYPE_UTIL) mysql

server-type-zabbix.sql: server-type
server-type-nagios.sql: server-type
if WITH_QPID
server-type-hapi-zabbix.sql: server-type
server-type-hapi-json.sql: server-type
server-type-ceilometer.sql: server-type
endif

HATOHOL_INIT_USER_GENERATOR = ../tools/hatohol-init-user-generator
init-user.sql: $(HATOHOL_INIT_USER_GENERATOR)
	$(HATOHOL_INIT_USER_GENERATOR) > _tmp.$@
	mv _tmp.$@ $@

pkgsysconf_DATA = \
	hatohol.conf
pkgsysconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = \
	$(pkgsysconf_DATA) \
	$(sql_DATA)

CLEANFILES = \
	server-type-zabbix.sql \
	server-type-nagios.sql

if WITH_QPID
CLEANFILES += \
	server-type-hapi-zabbix.sql \
	server-type-hapi-json.sql \
	server-type-ceilometer.sql
endif

CLEANFILES += \
	init-user.sql \
	_tmp.*
