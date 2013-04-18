SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = libTest.la testAsura.la
noinst_PROGRAMS = zabbix-api-response-collector
endif

# default
AM_CXXFLAGS = -g -Wall -D__STDC_FORMAT_MACROS
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
INCLUDES = \
	$(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) \
	$(GLIB_CFLAGS) $(MLPL_CFLAGS) $(JSON_GLIB_CFLAGS) $(LIBSOUP_CFLAGS) \
	-I ../src

LIBS = \
	$(CUTTER_LIBS) $(CPPCUTTER_LIBS) \
	$(GLIB_LIBS) $(MLPL_LIBS) ../src/libasura.la

# Test cases
testAsura_la_SOURCES = \
	testAsuraException.cc testSQLProcessorException.cc \
	testAsuraThreadBase.cc \
	testDBAgentSQLite3.cc testDBClientAsura.cc testDBClientZabbix.cc \
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
	testJsonParserAgent.cc testUtils.cc \
	testArmZabbixAPI.cc
testAsura_la_LDFLAGS = $(AM_LDFLAGS) $(LIBS)
testAsura_la_LIBADD = libTest.la

libTest_la_SOURCES = \
	FormulaTestUtils.cc Helpers.cc DBAgentTest.cc ZabbixAPIEmulator.cc \
	Synchronizer.cc
libTest_la_LDFLAGS =

zabbix_api_response_collector_LDFLAGS =
zabbix_api_response_collector_LDADD = ../src/libasura.la

echo-cutter:
	@echo $(CUTTER)
