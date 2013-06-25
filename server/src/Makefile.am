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
	ArmBase.cc ArmZabbixAPI.cc ArmNagiosNDOUtils.cc\
	\
	Hatohol.h HatoholThreadBase.h HatoholException.h \
	ConfigManager.h DBAgent.h DBAgentFactory.h \
	DBAgentSQLite3.h DBAgentMySQL.h \
	DBClient.h DBClientConfig.h DBClientHatohol.h DBClientZabbix.h \
	DBClientUtils.h \
	Utils.h FaceBase.h FaceMySQL.h FaceMySQLWorker.h \
	FaceRest.h \
	SQLProcessorTypes.h SQLProcessorException.h SQLUtils.h \
	SQLProcessorFactory.h SQLProcessor.h SQLProcessorZabbix.h \
	SQLProcessorSelect.h SQLProcessorInsert.h SQLProcessorUpdate.h \
	SQLFormulaParser.h SQLColumnParser.h SQLWhereParser.h \
	SQLFromParser.h SQLTableFormula.h \
	PrimaryCondition.h PrimaryConditionPicker.h \
	FormulaElement.h FormulaOperator.h FormulaFunction.h \
	UsedCountable.h \
	ItemGroupType.h ItemData.h ItemGroup.h ItemTable.h \
	ItemPtr.h ItemTablePtr.h ItemGroupPtr.h ItemDataUtils.h \
	ItemDataPtr.h ItemEnum.h ItemTableUtils.h ItemGroupEnum.h \
	VirtualDataStore.h VirtualDataStoreZabbix.h \
	VirtualDataStoreZabbixMacro.h \
	DataStoreException.h \
	DataStore.h DataStoreManager.h DataStoreZabbix.h \
	DataStoreNagios.h VirtualDataStoreNagios.h \
	JsonBuilderAgent.h JsonParserAgent.h \
	ArmBase.h ArmZabbixAPI.h ArmNagiosNDOUtils.h \
	ArmZabbixAPI-template.h

AM_CXXFLAGS = -Wall -g3 \
	-D__STDC_FORMAT_MACROS \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS) $(MYSQL_CFLAGS)

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS) $(MYSQL_LIBS)

hatohol_LDADD = libhatohol.la
