SUBDIRS = tls

sql_DATA = \
	create-db.sql server-type.sql
sqldir = $(pkgdatadir)/sql

server-type.sql: mk-server-type-sql
	PYTHONPATH=../tools ./mk-server-type-sql > $@

pkgsysconf_DATA = \
	hatohol.conf
pkgsysconfdir = $(sysconfdir)/$(PACKAGE)
