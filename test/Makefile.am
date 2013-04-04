SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = \
	testAsuraException.la testSQLProcessorException.la \
	testDBAgentSQLite3.la testDBClientZabbix.la \
	testSQLUtils.la testSQLProcessor.la \
	testSQLProcessorInsert.la testSQLProcessorUpdate.la \
	testFormulaElement.la \
	testSQLFromParser.la testSQLWhereParser.la \
	testSQLColumnParser.la \
	testMySQLWorkerZabbix.la testFaceRest.la \
	testItemData.la testItemGroup.la \
	testItemPtr.la testItemGroupType.la testItemTable.la \
	testItemTablePtr.la \
	testItemDataUtils.la \
	testFormulaOperator.la \
	testJsonParserAgent.la \
	libFormulaTestUtils.la libHelpers.la

endif

# default
AM_CFLAGS = -g -Wall $(GLIB_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
INCLUDES = \
	$(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) \
	$(GLIB_CFLAGS) $(MLPL_CFLAGS) $(JSON_GLIB_CFLAGS) $(LIBSOUP_CFLAGS) \
	-I ../src

LIBS = \
	$(CUTTER_LIBS) $(CPPCUTTER_LIBS) \
	$(GLIB_LIBS) $(MLPL_LIBS) ../src/libasura.la

# Test cases
testSQLProcessor_la_SOURCES = testSQLProcessor.cc
testSQLProcessor_la_LIBADD = ./libFormulaTestUtils.la ./libHelpers.la

testSQLColumnParser_la_SOURCES = testSQLColumnParser.cc
testSQLColumnParser_la_LIBADD = ./libFormulaTestUtils.la

testSQLWhereParser_la_SOURCES = testSQLWhereParser.cc
testSQLWhereParser_la_LIBADD = ./libFormulaTestUtils.la

testSQLProcessorInsert_la_SOURCES = testSQLProcessorInsert.cc
testSQLProcessorInsert_la_LIBADD = ./libHelpers.la

testSQLProcessorUpdate_la_SOURCES = testSQLProcessorUpdate.cc
testSQLProcessorUpdate_la_LIBADD = ./libHelpers.la ./libFormulaTestUtils.la

testDBAgentSQLite3_la_SOURCES = testDBAgentSQLite3.cc
testDBAgentSQLite3_la_LIBADD = ./libHelpers.la

testDBClientZabbix_la_SOURCES = testDBClientZabbix.cc
testDBClientZabbix_la_LIBADD = ./libHelpers.la

testFaceRest_la_SOURCES = testFaceRest.cc
testFaceRest_la_LIBADD = ./libHelpers.la

testMySQLWorkerZabbix_la_SOURCES = testMySQLWorkerZabbix.cc
testItemData_la_SOURCES = testItemData.cc
testItemGroup_la_SOURCES = testItemGroup.cc
testItemPtr_la_SOURCES = testItemPtr.cc
testItemGroupType_la_SOURCES = testItemGroupType.cc
testItemTable_la_SOURCES = testItemTable.cc
testItemTablePtr_la_SOURCES = testItemTablePtr.cc

testFormulaOperator_la_SOURCES = testFormulaOperator.cc
testFormulaOperator_la_LIBADD = ./libFormulaTestUtils.la

libFormulaTestUtils_la_SOURCES = FormulaTestUtils.cc
libFormulaTestUtils_la_LDFLAGS =

libHelpers_la_SOURCES = Helpers.cc DBAgentTest.cc
libHelpers_la_LDFLAGS =

echo-cutter:
	@echo $(CUTTER)
