SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = libTest.la testHatohol.la
noinst_PROGRAMS = ActionTp zabbix-api-response-collector
endif

# default
AM_CXXFLAGS = -g3 -Wall -D__STDC_FORMAT_MACROS
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
INCLUDES = \
	$(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) \
	$(GLIB_CFLAGS) $(MLPL_CFLAGS) $(JSON_GLIB_CFLAGS) $(LIBSOUP_CFLAGS) \
	$(MYSQL_CFLAGS) \
	-I ../src

LIBS = \
	$(CUTTER_LIBS) $(CPPCUTTER_LIBS) \
	$(GLIB_LIBS) $(GIO_LIBS) $(MLPL_LIBS) ../src/libhatohol.la

# Test cases
testHatohol_la_SOURCES = \
	testActionManager.cc \
	testHatoholException.cc testSQLProcessorException.cc \
	testHatoholThreadBase.cc \
	testDBAgentSQLite3.cc testDBAgentMySQL.cc \
	testDBClientUtils.cc \
	testDBClientConfig.cc testDBClientHatohol.cc testDBClientZabbix.cc \
	testDBClientAction.cc \
	testSQLUtils.cc testSQLProcessor.cc \
	testSQLProcessorInsert.cc testSQLProcessorUpdate.cc \
	testFormulaElement.cc \
	testSQLFromParser.cc testSQLWhereParser.cc \
	testSQLColumnParser.cc \
	testMySQLWorkerZabbix.cc testFaceRest.cc \
	testItemData.cc testItemGroup.cc \
	testItemPtr.cc testItemGroupType.cc testItemTable.cc \
	testItemTablePtr.cc \
	testItemDataUtils.cc \
	testFormulaOperator.cc \
	testJsonParserAgent.cc testJsonBuilderAgent.cc testUtils.cc \
	testArmZabbixAPI.cc testArmNagiosNDOUtils.cc \
	testUnifiedDataStore.cc
testHatohol_la_LDFLAGS = $(AM_LDFLAGS) $(LIBS)
testHatohol_la_LIBADD = libTest.la

libTest_la_SOURCES = \
	FormulaTestUtils.cc Helpers.cc DBAgentTest.cc DBClientTest.cc \
	ZabbixAPIEmulator.cc Synchronizer.cc \
	FormulaTestUtils.h Helpers.h DBAgentTest.h DBClientTest.h \
	ZabbixAPIEmulator.h Synchronizer.h \
	ExceptionTestUtils.h AssertJoin.h \
	PipeUtils.cc PipeUtils.h
libTest_la_LDFLAGS =

ActionTp_SOURCES = ActionTp.cc ActionTp.h
ActionTp_LDADD = -lTest

zabbix_api_response_collector_SOURCES = zabbix-api-response-collector.cc
zabbix_api_response_collector_LDFLAGS = -lstdc++
zabbix_api_response_collector_LDADD = $(LIBS)

echo-cutter:
	@echo $(CUTTER)
