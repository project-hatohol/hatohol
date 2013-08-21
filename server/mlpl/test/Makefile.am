#SUBDIRS = fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = testMLPL.la
noinst_PROGRAMS = loggerTestee
noinst_HEADERS = loggerTester.h

endif

# default
AM_CXXFLAGS = -g -Wall -O2 $(GLIB_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
INCLUDES = $(CUTTER_CFLAGS) $(CPPCUTTER_CFLAGS) -I ../src
LIBS = $(CUTTER_LIBS) $(CPPCUTTER_LIBS) -lstdc++ ../src/libmlpl.la

loggerTestee_SOURCES = loggerTestee.cc

# Test cases
testMLPL_la_SOURCES = \
	testMutexLock.cc testReadWriteLock.cc \
	testLogger.cc testStringUtils.cc testParsableString.cc \
	testSmartBuffer.cc

echo-cutter:
	@echo $(CUTTER)
