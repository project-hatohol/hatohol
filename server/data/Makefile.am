sql_DATA = \
	create-db.sql \
	server-type.sql \
	init-user.sql

sqldir = $(pkgdatadir)/sql

# To avoid making an incomplete .sql files, we once
# use a tmporary file: _tmp.$@
HATOHOL_SERVER_TYPE_UTIL = ../tools/hatohol-server-type-util
server-type.sql: $(HATOHOL_SERVER_TYPE_UTIL)
	$(HATOHOL_SERVER_TYPE_UTIL) mysql > _tmp.$@
	mv _tmp.$@ $@

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
	server-type.sql \
	init-user.sql \
	_tmp.*
