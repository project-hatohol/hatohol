if WITH_CUTTER
all: testTriggerList.db FooTable.db
noinst_PROGRAMS = mkTestDB
endif

AM_CXXFLAGS = -g -Wall -I .. -I ../../src $(GLIB_CFLAGS)
LIBS = ../../src/libasura.la ../libHelpers.la

mkTestDB_SOURCES = mkTestDB.cc

testTriggerList.db: mkTestDB
	rm -f $@
	mkTestDB $@

FooTable.db: mkFooTable.sh
	rm -f $@
	mkFooTable.sh $@
