#SUBDIRS = fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = \
	testSQLProcessor.la testSQLWhereElement.la testMySQLWorkerZabbix.la \
	testItemData.la testItemGroup.la \
	testItemPtr.la testItemGroupType.la testItemTable.la \
	testItemTablePtr.la

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
testSQLWhereElement_la_SOURCES = testSQLWhereElement.cc
testMySQLWorkerZabbix_la_SOURCES = testMySQLWorkerZabbix.cc
testItemData_la_SOURCES = testItemData.cc
testItemGroup_la_SOURCES = testItemGroup.cc
testItemPtr_la_SOURCES = testItemPtr.cc
testItemGroupType_la_SOURCES = testItemGroupType.cc
testItemTable_la_SOURCES = testItemTable.cc
testItemTablePtr_la_SOURCES = testItemTablePtr.cc

echo-cutter:
	@echo $(CUTTER)
