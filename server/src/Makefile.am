sbin_PROGRAMS = hatohol hatohol-resident-yard
hatohol_SOURCES = main.cc
hatohol_resident_yard_SOURCES = hatoholResidentYard.cc

lib_LTLIBRARIES = libhatohol.la

libhatohol_la_SOURCES = \
	ActionExecArgMaker.cc ActionExecArgMaker.h \
	ActionManager.cc ActionManager.h \
	ActorCollector.cc ActorCollector.h \
	ArmUtils.cc ArmUtils.h \
	ArmBase.cc ArmBase.h \
	ArmFake.cc ArmFake.h \
	ArmIncidentTracker.cc ArmIncidentTracker.h \
	ArmNagiosNDOUtils.cc ArmNagiosNDOUtils.h \
	ArmRedmine.cc ArmRedmine.h \
	ArmZabbixAPI.cc ArmZabbixAPI.h \
	ChildProcessManager.cc ChildProcessManager.h \
	Closure.h \
	ThreadLocalDBCache.cc ThreadLocalDBCache.h \
	ConfigManager.cc ConfigManager.h \
	DataQueryContext.cc DataQueryContext.h \
	DataQueryOption.cc DataQueryOption.h \
	DBAgent.cc DBAgent.h \
	DBAgentFactory.cc DBAgentFactory.h \
	DBAgentMySQL.cc DBAgentMySQL.h \
	DBAgentSQLite3.cc DBAgentSQLite3.h \
	DB.cc DB.h \
	DBHatohol.cc DBHatohol.h \
	DBTables.cc DBTables.h \
	DBTablesAction.cc DBTablesAction.h \
	DBTablesConfig.cc DBTablesConfig.h \
	DBTablesUser.cc DBTablesUser.h \
	DBTablesMonitoring.cc DBTablesMonitoring.h \
	DBTablesHost.cc DBTablesHost.h \
	DBTablesLastInfo.cc DBTablesLastInfo.h \
	DBClientJoinBuilder.cc DBClientJoinBuilder.h \
	DBTermCodec.h DBTermCodec.cc \
	DBTermCStringProvider.h DBTermCStringProvider.cc \
	DataStore.cc DataStore.h \
	DataStoreFactory.cc DataStoreFactory.h \
	DataStoreManager.cc DataStoreManager.h \
	DataStoreFake.cc DataStoreFake.h \
	DataStoreNagios.cc DataStoreNagios.h \
	DataStoreZabbix.cc DataStoreZabbix.h \
	FaceBase.cc FaceBase.h \
	FaceRest.cc FaceRest.h \
	FaceRestPrivate.h \
	Hatohol.cc Hatohol.h \
	HostResourceQueryOption.cc HostResourceQueryOption.h \
	HatoholArmPluginGate.cc HatoholArmPluginGate.h \
	HatoholServer.cc \
	HatoholDBUtils.cc HatoholDBUtils.h \
	HostInfoCache.cc HostInfoCache.h \
	IncidentSender.cc IncidentSender.h \
	IncidentSenderManager.cc IncidentSenderManager.h \
	IncidentSenderRedmine.cc IncidentSenderRedmine.h \
	ItemFetchWorker.cc ItemFetchWorker.h \
	ItemGroupStream.cc ItemGroupStream.h \
	ItemGroupEnum.h \
	ItemTableUtils.h \
	LabelUtils.cc LabelUtils.h \
	OperationPrivilege.cc OperationPrivilege.h \
	RedmineAPI.cc RedmineAPI.h \
	ResidentProtocol.h \
	ResidentCommunicator.cc ResidentCommunicator.h \
	RestResourceAction.cc RestResourceAction.h \
	RestResourceHost.cc RestResourceHost.h \
	RestResourceIncidentTracker.cc RestResourceIncidentTracker.h \
	RestResourceServer.cc RestResourceServer.h \
	RestResourceUser.cc RestResourceUser.h \
	SessionManager.cc SessionManager.h \
	SQLProcessorTypes.h \
	SQLUtils.cc SQLUtils.h \
	TriggerFetchWorker.cc TriggerFetchWorker.h \
	UnifiedDataStore.cc UnifiedDataStore.h

if HAVE_LIBRABBITMQ
libhatohol_la_SOURCES += \
	GateJSONEventMessage.cc GateJSONEventMessage.h \
	HatoholArmPluginGateJSON.cc HatoholArmPluginGateJSON.h \
	HatoholArmPluginGateHAPI2.cc HatoholArmPluginGateHAPI2.h
endif

AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GTHREAD_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS) \
	$(UUID_CFLAGS) $(LIBRABBITMQ_CFLAGS) $(MYSQL_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I $(top_srcdir)/server/common

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GTHREAD_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS) \
	$(UUID_LIBS) $(LIBRABBITMQ_LIBS) $(MYSQL_LIBS) \
	$(COVERAGE_LIBS)

libhatohol_la_LDFLAGS = $(AM_LDFLAGS)
libhatohol_la_LIBADD = $(top_builddir)/server/common/libhatohol-common.la
hatohol_LDADD = \
	libhatohol.la \
	$(top_builddir)/server/common/libhatohol-common.la
hatohol_resident_yard_LDADD = \
	libhatohol.la \
	$(top_builddir)/server/common/libhatohol-common.la

$(top_builddir)/server/common/libhatohol-common.la:
	$(MAKE) -C $(top_builddir)/server/common
