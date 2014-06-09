sbin_PROGRAMS = hatohol hatohol-resident-yard
hatohol_SOURCES = main.cc
hatohol_resident_yard_SOURCES = hatoholResidentYard.cc

lib_LTLIBRARIES = libhatohol-common.la libhatohol.la

libhatohol_common_la_SOURCES = \
	DataStoreException.cc DataStoreException.h \
	HatoholThreadBase.cc HatoholThreadBase.h \
	HatoholArmPluginInterface.cc HatoholArmPluginInterface.h \
	HatoholArmPluginBase.cc HatoholArmPluginBase.h \
	HatoholException.cc HatoholException.h \
	HapZabbixAPI.cc HapZabbixAPI.h \
	ItemData.cc ItemData.h \
	ItemDataUtils.cc ItemDataUtils.h \
	ItemGroup.cc ItemGroup.h \
	ItemGroupType.cc ItemGroupType.h \
	ItemGroupPtr.cc ItemGroupPtr.h \
	ItemTable.cc ItemTable.h \
	ItemTablePtr.cc ItemTablePtr.h \
	JsonBuilderAgent.cc JsonBuilderAgent.h \
	JsonParserAgent.cc JsonParserAgent.h \
	MonitoringServerInfo.cc MonitoringServerInfo.h \
	UsedCountable.cc UsedCountable.h \
	Utils.cc Utils.h \
	UsedCountablePtr.cc UsedCountablePtr.h \
	ZabbixAPI.cc ZabbixAPI.h

libhatohol_la_SOURCES = \
	ActionExecArgMaker.cc ActionExecArgMaker.h \
	ActionManager.cc ActionManager.h \
	ActorCollector.cc ActorCollector.h \
	ArmBase.cc ArmBase.h \
	ArmFake.cc ArmFake.h \
	ArmNagiosNDOUtils.cc ArmNagiosNDOUtils.h \
	ArmZabbixAPI-template.h \
	ArmZabbixAPI.cc ArmZabbixAPI.h \
	ArmCeilometer.cc ArmCeilometer.h \
	ArmStatus.cc ArmStatus.h \
	ChildProcessManager.cc ChildProcessManager.h \
	Closure.cc Closure.h \
	CacheServiceDBClient.cc CacheServiceDBClient.h \
	ConfigManager.cc ConfigManager.h \
	DataQueryContext.cc DataQueryContext.h \
	DataQueryOption.cc DataQueryOption.h \
	DBAgent.cc DBAgent.h \
	DBAgentFactory.cc DBAgentFactory.h \
	DBAgentMySQL.cc DBAgentMySQL.h \
	DBAgentSQLite3.cc DBAgentSQLite3.h \
	DBClient.cc DBClient.h \
	DBClientAction.cc DBClientAction.h \
	DBClientConfig.cc DBClientConfig.h \
	DBClientHatohol.cc DBClientHatohol.h \
	DBClientUser.h DBClientUser.cc \
	DBClientZabbix.cc DBClientZabbix.h \
	DBTermCodec.h DBTermCodec.cc \
	DataStore.cc DataStore.h \
	DataStoreFactory.cc DataStoreFactory.h \
	DataStoreManager.cc DataStoreManager.h \
	DataStoreFake.cc DataStoreFake.h \
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
	HatoholError.cc HatoholError.h \
	HostResourceQueryOption.cc HostResourceQueryOption.h \
	HatoholArmPluginGate.cc HatoholArmPluginGate.h \
	HapgZabbixAPI.cc HapgZabbixAPI.h \
	HatoholServer.cc \
	ItemData.cc ItemData.h \
	ItemDataPtr.h \
	ItemEnum.h \
	ItemFetchWorker.cc ItemFetchWorker.h \
	ItemGroupStream.cc ItemGroupStream.h \
	ItemGroupEnum.h \
	ItemTableUtils.h \
	LabelUtils.cc LabelUtils.h \
	NamedPipe.cc NamedPipe.h \
	OperationPrivilege.cc OperationPrivilege.h \
	Params.h \
	PrimaryCondition.cc PrimaryCondition.h \
	PrimaryConditionPicker.cc PrimaryConditionPicker.h \
	ResidentProtocol.h \
	ResidentCommunicator.cc ResidentCommunicator.h \
	SessionManager.cc SessionManager.h \
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
	SQLTableFormula.cc SQLTableFormula.h \
	SQLUtils.cc SQLUtils.h \
	SQLWhereParser.cc SQLWhereParser.h \
	UnifiedDataStore.cc UnifiedDataStore.h

AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS) \
	$(UUID_CFLAGS) $(MYSQL_CFLAGS) $(COVERAGE_CFLAGS)

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS) \
	$(UUID_LIBS) $(MYSQL_LIBS) $(COVERAGE_LIBS)

libhatohol_la_LDFLAGS = $(AM_LDFLAGS) libhatohol-common.la
hatohol_LDADD = libhatohol.la libhatohol-common.la
hatohol_resident_yard_LDADD = libhatohol.la libhatohol-common.la
