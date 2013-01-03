#SUBDIRS = fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = \
	testSQLProcessor.la

endif

# default
AM_CFLAGS = -g -Wall $(GLIB_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
INCLUDES = $(CUTTER_CFLAGS) $(GLIB_CFLAGS) $(MLPL_CFLAGS) -I ../src
LIBS = $(CUTTER_LIBS) $(GLIB_LIBS) $(MLPL_CFLAGS) ../src/libasura.la

# Test cases
testSQLProcessor_la_SOURCES = testSQLProcessor.cc

echo-cutter:
	@echo $(CUTTER)
