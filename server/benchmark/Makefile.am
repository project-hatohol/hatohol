AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(HFL_CFLAGS) \
	$(GLIB_CFLAGS) \
	-I../src -I ../common

AM_LDFLAGS = -lrt \
	$(HFL_LIBS) \
	$(GLIB_LIBS)

noinst_PROGRAMS = \
	bench-string-join

bench_string_join_SOURCES = bench-string-join.cc

run-bench-string-join: bench-string-join
	./$<
