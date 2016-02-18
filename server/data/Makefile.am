sql_DATA = \
	init-user.sql \
	server-type-hapi-json.sql \
	server-type-hap2-zabbix.sql \
	server-type-hap2-nagios-ndoutils.sql \
	server-type-hap2-nagios-livestatus.sql \
	server-type-hap2-ceilometer.sql \
	server-type-hap2-fluentd.sql \
	severity-ranks.sql \
	custom-incident-statuses.sql \
	incident-trackers.sql \
	actions.sql

noinst_DATA = create-db.sql

sqldir = $(pkgdatadir)/sql

HATOHOL_SERVER_TYPE_UTIL = ../tools/hatohol-server-type-util
server-type: $(HATOHOL_SERVER_TYPE_UTIL)
	$(HATOHOL_SERVER_TYPE_UTIL) mysql

server-type-hapi-json.sql: server-type

HATOHOL_INIT_USER_GENERATOR = ../tools/hatohol-init-user-generator
init-user.sql: $(HATOHOL_INIT_USER_GENERATOR)
	$(HATOHOL_INIT_USER_GENERATOR) > _tmp.$@
	mv _tmp.$@ $@

pkgsysconf_DATA = \
	hatohol.conf
pkgsysconfdir = $(sysconfdir)/$(PACKAGE)

EXTRA_DIST = \
	$(pkgsysconf_DATA) \
	$(sql_DATA) \
	$(noinst_DATA)

CLEANFILES = \
	server-type-hapi-json.sql \
	init-user.sql \
	_tmp.*
