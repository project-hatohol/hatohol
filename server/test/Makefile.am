SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = libTest.la testHatohol.la residentTest.la
noinst_PROGRAMS = ActionTp zabbix-api-response-collector hapi-test-plugin
endif

# default
AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) \
	$(GLIB_CFLAGS) $(MLPL_CFLAGS) $(JSON_GLIB_CFLAGS) $(LIBSOUP_CFLAGS) \
	$(MYSQL_CFLAGS) \
	-I ../src
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

LIBS = \
	$(CUTTER_LIBS) $(CPPCUTTER_LIBS) \
	$(GLIB_LIBS) $(GIO_LIBS) $(MLPL_LIBS) ../src/libhatohol.la ../src/libhatohol-common.la

# Test cases
testHatohol_la_SOURCES = \
	testActionExecArgMaker.cc testActionManager.cc \
	testActorCollector.cc \
	testCacheServiceDBClient.cc \
	testChildProcessManager.cc \
	testConfigManager.cc \
	testDataQueryContext.cc testDataQueryOption.cc \
	testDataStoreManager.cc testDataStoreFactory.cc \
	testDataStoreZabbix.cc testDataStoreNagios.cc \
	testHatoholArmPluginInterface.cc testHatoholArmPluginGate.cc \
	testHatoholArmPluginBase.cc \
	testHapZabbixAPI.cc \
	testHatoholError.cc \
	testHatoholException.cc testSQLProcessorException.cc \
	testHatoholThreadBase.cc \
	testHostInfoCache.cc \
	testHostResourceQueryOption.cc \
	testHostResourceQueryOptionSubClasses.cc \
	testDBAgent.cc \
	testDBAgentSQLite3.cc testDBAgentMySQL.cc \
	testDBClient.cc \
	testDBClientUtils.cc \
	testDBClientConfig.cc \
	testDBClientHatohol.cc testDBClientHatohol.h \
	testDBClientZabbix.cc \
	testDBClientAction.cc testDBClientUser.cc \
	testDBTermCodec.cc \
	testOperationPrivilege.cc \
	testSQLUtils.cc testSQLProcessor.cc \
	testSQLProcessorInsert.cc testSQLProcessorUpdate.cc \
	testFormulaElement.cc \
	testSQLFromParser.cc testSQLWhereParser.cc \
	testSQLColumnParser.cc \
	testMySQLWorkerZabbix.cc testFaceRest.cc \
	testSessionManager.cc \
	testIssueSenderRedmine.cc \
	testIssueSenderManager.cc \
	testItemData.cc testItemGroup.cc testItemGroupStream.cc \
	testItemDataPtr.cc testItemGroupType.cc testItemTable.cc \
	testItemTablePtr.cc \
	testItemDataUtils.cc \
	testFormulaOperator.cc \
	testJsonParserAgent.cc testJsonBuilderAgent.cc testUtils.cc \
	testNamedPipe.cc \
	testArmBase.cc \
	testArmZabbixAPI.cc testArmNagiosNDOUtils.cc \
	testArmStatus.cc \
	testUsedCountable.cc \
	testUnifiedDataStore.cc testMain.cc \
	testZabbixAPI.cc \
	test_hatohol-arm-plugin-zabbix.cc

testHatohol_la_LDFLAGS = $(AM_LDFLAGS) $(LIBS)
testHatohol_la_LIBADD = libTest.la

residentTest_la_SOURCES = residentTest.cc residentTest.h
residentTest_la_LDFLAGS = $(AM_LDFLAGS) $(LIBS)
residentTest_la_LIBADD  = libTest.la

libTest_la_SOURCES = \
	DataSamples.cc DataSamples.h \
	FormulaTestUtils.cc Helpers.cc DBAgentTest.cc DBClientTest.cc \
	ZabbixAPITestUtils.cc ZabbixAPITestUtils.h \
	HttpServerStub.cc ZabbixAPIEmulator.cc RedmineAPIEmulator.cc \
	Synchronizer.cc \
	FormulaTestUtils.h Helpers.h DBAgentTest.h DBClientTest.h \
	HttpServerStub.h ZabbixAPIEmulator.h RedmineAPIEmulator.h \
	Synchronizer.h \
	ExceptionTestUtils.h AssertJoin.h \
	MultiLangTest.h \
	HatoholArmPluginInterfaceTest.cc HatoholArmPluginInterfaceTest.h \
	HatoholArmPluginGateTest.cc HatoholArmPluginGateTest.h \
	HatoholArmPluginTestPair.cc HatoholArmPluginTestPair.h
libTest_la_LDFLAGS =

ActionTp_SOURCES = ActionTp.cc ActionTp.h
ActionTp_LDFLAGS = $(LIBS)
ActionTp_LDADD = libTest.la

zabbix_api_response_collector_SOURCES = zabbix-api-response-collector.cc
zabbix_api_response_collector_LDFLAGS = -lstdc++
zabbix_api_response_collector_LDADD = $(LIBS)

hapi_test_plugin_SOURCES = hapi-test-plugin.cc
hapi_test_plugin_LDFLAGS = $(LIBS) -lqpidmessaging

echo-cutter:
	@echo $(CUTTER)
