AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) \
	$(GLIB_CFLAGS) \
	-I $(top_srcdir)/server/src \
	-I $(top_srcdir)/server/common

AM_LDFLAGS = -lrt \
	$(MLPL_LIBS) \
	$(GLIB_LIBS)

noinst_PROGRAMS = \
	bench-string-join

bench_string_join_SOURCES = bench-string-join.cc

run-bench-string-join: bench-string-join
	./$<
