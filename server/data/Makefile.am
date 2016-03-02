sql_DATA = \
	10-init-user.sql \
	30-severity-ranks.sql \
	30-custom-incident-statuses.sql \
	50-incident-trackers.sql \
	55-actions.sql \
	90-server-type-hapi-json.sql \
	90-server-type-hap2-zabbix.sql \
	90-server-type-hap2-nagios-ndoutils.sql \
	90-server-type-hap2-nagios-livestatus.sql \
	90-server-type-hap2-ceilometer.sql \
	90-server-type-hap2-fluentd.sql

noinst_DATA = create-db.sql

sqldir = $(pkgdatadir)/sql

HATOHOL_SERVER_TYPE_UTIL = ../tools/hatohol-server-type-util
server-type: $(HATOHOL_SERVER_TYPE_UTIL)
	$(HATOHOL_SERVER_TYPE_UTIL) mysql

90-server-type-hapi-json.sql: server-type

HATOHOL_INIT_USER_GENERATOR = ../tools/hatohol-init-user-generator
10-init-user.sql: $(HATOHOL_INIT_USER_GENERATOR)
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
	90-server-type-hapi-json.sql \
	10-init-user.sql \
	_tmp.*
