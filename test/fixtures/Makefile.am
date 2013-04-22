TEST_DBS = testDatabase-config.db testDatabase-asura.db
if WITH_CUTTER
noinst_DATA = $(TEST_DBS) FooTable.db
noinst_PROGRAMS = mkTestDB
endif

AM_CXXFLAGS = -g -Wall -I .. -I ../../src $(GLIB_CFLAGS) $(MLPL_CFLAGS)
LIBS = ../../src/libasura.la ../libTest.la

mkTestDB_SOURCES = mkTestDB.cc

$(TEST_DBS): mkTestDB ../../src/libasura.la
testDatabase-config.db:
	rm -f $@
	mkTestDB config $@

testDatabase-asura.db:
	rm -f $@
	mkTestDB asura $@

FooTable.db: mkFooTable.sh
	rm -f $@
	mkFooTable.sh $@
