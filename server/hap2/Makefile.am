SUBDIRS = hatohol
hatohol_hap2dir=$(libexecdir)/$(PACKAGE)/hap2

noinst_SCRIPTS = \
	setup_common.py \
	setup_zabbixapi.py \
	setup_rabbitmqconnector.py

install-data-hook:
	python setup_common.py install --root=$(DESTDIR)
	python setup_zabbixapi.py install --root=$(DESTDIR)
	python setup_rabbitmqconnector.py install --root=$(DESTDIR)

nobase_dist_hatohol_hap2_DATA = \
	hatohol/hap2_zabbix_api.py \
	hatohol/hap2_fluentd.py \
	hatohol/hap2_ceilometer.py \
	hatohol/hap2_nagios_ndoutils.py

EXTRA_DIST = $(noinst_SCRIPTS)
