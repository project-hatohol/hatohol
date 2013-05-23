sbin_PROGRAMS = asura
asura_SOURCES = main.cc

lib_LTLIBRARIES = libasura.la
libasura_la_SOURCES = \
	Asura.cc AsuraThreadBase.cc AsuraException.cc \
	ConfigManager.cc DBAgent.cc DBAgentFactory.cc \
	DBAgentSQLite3.cc DBAgentMySQL.cc \
	DBClient.cc DBClientConfig.cc DBClientAsura.cc DBClientZabbix.cc \
	Utils.cc FaceBase.cc FaceMySQL.cc FaceMySQLWorker.cc \
	FaceRest.cc \
	SQLProcessorTypes.cc SQLProcessorException.cc SQLUtils.cc \
	SQLProcessorFactory.cc SQLProcessor.cc SQLProcessorZabbix.cc \
	SQLProcessorSelect.cc SQLProcessorInsert.cc SQLProcessorUpdate.cc \
	SQLFormulaParser.cc SQLColumnParser.cc SQLWhereParser.cc \
	SQLFromParser.cc SQLTableFormula.cc \
	PrimaryCondition.cc PrimaryConditionPicker.cc \
	FormulaElement.cc FormulaOperator.cc FormulaFunction.cc \
	UsedCountable.cc \
	ItemGroupType.cc ItemData.cc ItemGroup.cc ItemTable.cc \
	ItemPtr.cc ItemTablePtr.cc ItemGroupPtr.cc ItemDataUtils.cc \
	VirtualDataStore.cc VirtualDataStoreZabbix.cc \
	VirtualDataStoreZabbixProfiles.cc \
	DataStoreException.cc \
	DataStore.cc DataStoreManager.cc DataStoreZabbix.cc \
	JsonBuilderAgent.cc JsonParserAgent.cc \
	ArmBase.cc ArmZabbixAPI.cc

AM_CXXFLAGS = -Wall -g3 \
	-D__STDC_FORMAT_MACROS \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS) $(MYSQL_CFLAGS)

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS)

asura_LDADD = libasura.la
