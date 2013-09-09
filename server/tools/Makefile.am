bin_PROGRAMS = hatohol-config-db-creator hatohol-cli
hatohol_config_db_creator_SOURCES = hatohol-config-db-creator.cc
hatohol_cli_SOURCES = hatohol-cli.cc

MLPL_CFLAGS = -I ../mlpl/src
MLPL_LIBS = ../mlpl/src/libmlpl.la

AM_CXXFLAGS = -Wall -g3 \
	-D__STDC_FORMAT_MACROS \
	-I ../src $(MLPL_CFLAGS) $(GLIB_CFLAGS) $(MYSQL_CFLAGS)

AM_LDFLAGS = ../src/libhatohol.la $(MLPL_LIBS) $(GLIB_LIBS) $(MYSQL_LIBS)

config_example_DATA = hatohol-config.dat.example
config_exampledir = $(pkgdatadir)

EXTRA_DIST = $(config_example_DATA)
