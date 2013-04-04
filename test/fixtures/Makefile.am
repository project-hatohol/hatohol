if WITH_CUTTER
noinst_DATA = testDatabase.db FooTable.db
noinst_PROGRAMS = mkTestDB
endif

AM_CXXFLAGS = -g -Wall -I .. -I ../../src $(GLIB_CFLAGS) $(MLPL_CFLAGS)
LIBS = ../../src/libasura.la ../libTest.la

mkTestDB_SOURCES = mkTestDB.cc

testDatabase.db: mkTestDB ../../src/libasura.la
	rm -f $@
	mkTestDB $@

FooTable.db: mkFooTable.sh
	rm -f $@
	mkFooTable.sh $@
