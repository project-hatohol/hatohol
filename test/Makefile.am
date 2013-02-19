#SUBDIRS = fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = \
	testAsuraException.la testSQLProcessorException.la \
	testSQLUtils.la testSQLProcessor.la \
	testSQLProcessorInsert.la testSQLProcessorUpdate.la \
	testFormulaElement.la \
	testSQLWhereParser.la \
	testSQLColumnParser.la testMySQLWorkerZabbix.la \
	testItemData.la testItemGroup.la \
	testItemPtr.la testItemGroupType.la testItemTable.la \
	testItemTablePtr.la \
	testItemDataUtils.la \
	testFormulaOperator.la \
	libFormulaTestUtils.la libHelpers.la

endif

# default
AM_CFLAGS = -g -Wall $(GLIB_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
INCLUDES = \
	$(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) \
	$(GLIB_CFLAGS) $(MLPL_CFLAGS) -I ../src

LIBS = \
	$(CUTTER_LIBS) $(CPPCUTTER_LIBS) \
	$(GLIB_LIBS) $(MLPL_LIBS) ../src/libasura.la

# Test cases
testSQLProcessor_la_SOURCES = testSQLProcessor.cc
testSQLProcessor_la_LIBADD = ./libFormulaTestUtils.la

testSQLColumnParser_la_SOURCES = testSQLColumnParser.cc
testSQLColumnParser_la_LIBADD = ./libFormulaTestUtils.la

testSQLWhereParser_la_SOURCES = testSQLWhereParser.cc
testSQLWhereParser_la_LIBADD = ./libFormulaTestUtils.la

testSQLProcessorInsert_la_SOURCES = testSQLProcessorInsert.cc
testSQLProcessorInsert_la_LIBADD = ./libHelpers.la

testSQLProcessorUpdate_la_SOURCES = testSQLProcessorUpdate.cc
testSQLProcessorUpdate_la_LIBADD = ./libHelpers.la ./libFormulaTestUtils.la

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

libHelpers_la_SOURCES = Helpers.cc
libHelpers_la_LDFLAGS =

echo-cutter:
	@echo $(CUTTER)
