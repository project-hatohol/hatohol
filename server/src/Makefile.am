sbin_PROGRAMS = hatohol
hatohol_SOURCES = main.cc

lib_LTLIBRARIES = libhatohol.la
libhatohol_la_SOURCES = \
	ArmBase.cc ArmBase.h \
	ArmNagiosNDOUtils.cc ArmNagiosNDOUtils.h \
	ArmZabbixAPI-template.h \
	ArmZabbixAPI.cc ArmZabbixAPI.h \
	Closure.cc Closure.h \
	ConfigManager.cc ConfigManager.h \
	DBAgent.cc DBAgent.h \
	DBAgentFactory.cc DBAgentFactory.h \
	DBAgentMySQL.cc DBAgentMySQL.h \
	DBAgentSQLite3.cc DBAgentSQLite3.h \
	DBClient.cc DBClient.h \
	DBClientConfig.cc DBClientConfig.h \
	DBClientHatohol.cc DBClientHatohol.h \
	DBClientUtils.h \
	DBClientZabbix.cc DBClientZabbix.h \
	DataStore.cc DataStore.h \
	DataStoreException.cc DataStoreException.h \
	DataStoreManager.cc DataStoreManager.h \
	DataStoreNagios.cc DataStoreNagios.h \
	DataStoreZabbix.cc DataStoreZabbix.h \
	FaceBase.cc FaceBase.h \
	FaceMySQL.cc FaceMySQL.h \
	FaceMySQLWorker.cc FaceMySQLWorker.h \
	FaceRest.cc FaceRest.h \
	FormulaElement.cc FormulaElement.h \
	FormulaFunction.cc FormulaFunction.h \
	FormulaOperator.cc FormulaOperator.h \
	Hatohol.cc Hatohol.h \
	HatoholException.cc HatoholException.h \
	HatoholThreadBase.cc HatoholThreadBase.h \
	ItemData.cc ItemData.h \
	ItemDataPtr.h \
	ItemDataUtils.cc ItemDataUtils.h \
	ItemEnum.h \
	ItemGroup.cc ItemGroup.h \
	ItemGroupEnum.h \
	ItemGroupPtr.cc ItemGroupPtr.h \
	ItemGroupType.cc ItemGroupType.h \
	ItemPtr.cc ItemPtr.h \
	ItemTable.cc ItemTable.h \
	ItemTablePtr.cc ItemTablePtr.h \
	ItemTableUtils.h \
	JsonBuilderAgent.cc JsonBuilderAgent.h \
	JsonParserAgent.cc JsonParserAgent.h \
	PrimaryCondition.cc PrimaryCondition.h \
	PrimaryConditionPicker.cc PrimaryConditionPicker.h \
	SQLColumnParser.cc SQLColumnParser.h \
	SQLFormulaParser.cc SQLFormulaParser.h \
	SQLFromParser.cc SQLFromParser.h \
	SQLProcessor.cc SQLProcessor.h \
	SQLProcessorException.cc SQLProcessorException.h \
	SQLProcessorFactory.cc SQLProcessorFactory.h \
	SQLProcessorInsert.cc SQLProcessorInsert.h \
	SQLProcessorSelect.cc SQLProcessorSelect.h \
	SQLProcessorTypes.cc SQLProcessorTypes.h \
	SQLProcessorUpdate.cc SQLProcessorUpdate.h \
	SQLProcessorZabbix.cc SQLProcessorZabbix.h \
	SQLTableFormula.cc SQLTableFormula.h \
	SQLUtils.cc SQLUtils.h \
	SQLWhereParser.cc SQLWhereParser.h \
	UnifiedDataStore.cc UnifiedDataStore.h \
	UsedCountable.cc UsedCountable.h \
	Utils.cc Utils.h \
	VirtualDataStore.cc VirtualDataStore.h \
	VirtualDataStoreNagios.cc VirtualDataStoreNagios.h \
	VirtualDataStoreZabbix.cc VirtualDataStoreZabbix.h \
	VirtualDataStoreZabbixProfiles.cc VirtualDataStoreZabbixMacro.h

AM_CXXFLAGS = -Wall -g3 \
	-D__STDC_FORMAT_MACROS \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS) $(MYSQL_CFLAGS)

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS)

hatohol_LDADD = libhatohol.la
