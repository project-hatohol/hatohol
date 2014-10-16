if WITH_CUTTER
noinst_DATA = FooTable.db
endif

noinst_SCRIPTS = mkFooTable.sh
FooTable.db: mkFooTable.sh
	rm -f $@
	$(srcdir)/mkFooTable.sh $@

EXTRA_DIST= \
	$(noinst_SCRIPTS) \
	testJSON01.json \
	testJSON02.json \
	testJSON03.json \
	testJSON04.json \
	testJSON05.json \
	testJSON06.json \
	zabbix-api-res-applications-003.json \
	zabbix-api-res-events-002.json \
	zabbix-api-res-hosts-001.json \
	zabbix-api-res-items-001.json \
	zabbix-api-res-items-003.json \
	zabbix-api-res-login-001.json \
	zabbix-api-res-triggers-001.json \
	zabbix-api-res-triggers-003-hosts.json \
	zabbix-api-res-hostgroup-001.json \
	zabbix-api-res-hostgroup-002-refer.json
