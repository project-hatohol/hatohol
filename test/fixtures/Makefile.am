if WITH_CUTTER
all: testDatabase.db FooTable.db
noinst_PROGRAMS = mkTestDB
endif

AM_CXXFLAGS = -g -Wall -I .. -I ../../src $(GLIB_CFLAGS)
LIBS = ../../src/libasura.la ../libHelpers.la

mkTestDB_SOURCES = mkTestDB.cc

testDatabase.db: mkTestDB ../../src/libasura.la
	rm -f $@
	mkTestDB $@

FooTable.db: mkFooTable.sh
	rm -f $@
	mkFooTable.sh $@
