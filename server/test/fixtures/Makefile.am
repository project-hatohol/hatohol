TEST_DBS = testDatabase-config.db testDatabase-hatohol.db
if WITH_CUTTER
noinst_DATA = $(TEST_DBS) FooTable.db
noinst_PROGRAMS = mkTestDB
endif

MLPL_CFLAGS = -I ../../mlpl/src
MLPL_LIBS = ../..//mlpl/src/libmlpl.la

AM_CXXFLAGS = -g3 -Wall -I .. -I ../../src $(GLIB_CFLAGS) $(MLPL_CFLAGS)
LIBS = ../../src/libhatohol.la ../libTest.la

mkTestDB_SOURCES = mkTestDB.cc

$(TEST_DBS): mkTestDB ../../src/libhatohol.la ../libTest.la
testDatabase-config.db:
	rm -f $@
	./mkTestDB config $@

testDatabase-hatohol.db:
	rm -f $@
	./mkTestDB hatohol $@

FooTable.db: mkFooTable.sh
	rm -f $@
	./mkFooTable.sh $@
