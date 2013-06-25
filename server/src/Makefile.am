sbin_PROGRAMS = hatohol
hatohol_SOURCES = main.cc

lib_LTLIBRARIES = libhatohol.la
libhatohol_la_SOURCES = \
	Hatohol.cc HatoholThreadBase.cc HatoholException.cc \
	ConfigManager.cc DBAgent.cc DBAgentFactory.cc \
	DBAgentSQLite3.cc DBAgentMySQL.cc \
	DBClient.cc DBClientConfig.cc DBClientHatohol.cc DBClientZabbix.cc \
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
	DataStoreNagios.cc VirtualDataStoreNagios.cc \
	JsonBuilderAgent.cc JsonParserAgent.cc \
	ArmBase.cc ArmZabbixAPI.cc ArmNagiosNDOUtils.cc

MLPL_CFLAGS = -I ../mlpl/src
MLPL_LIBS = ../mlpl/src/libmlpl.la

AM_CXXFLAGS = -Wall -g3 \
	-D__STDC_FORMAT_MACROS \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS) $(MYSQL_CFLAGS)

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS)

hatohol_LDADD = libhatohol.la
