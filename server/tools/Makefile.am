bin_PROGRAMS = hatohol-config-db-creator \
               hatohol-def-src-file-generator
actiondir = $(libexecdir)/$(PACKAGE)/action
dist_action_SCRIPTS = hatohol-actor-mail
hatohol_config_db_creator_SOURCES = hatohol-config-db-creator.cc

MLPL_CFLAGS = -I ../mlpl/src
MLPL_LIBS = ../mlpl/src/libmlpl.la

AM_CXXFLAGS = -Wall -g3 \
	-D__STDC_FORMAT_MACROS \
	-I ../src $(MLPL_CFLAGS) $(GLIB_CFLAGS) $(MYSQL_CFLAGS)

AM_LDFLAGS = ../src/libhatohol.la $(MLPL_LIBS) $(GLIB_LIBS) $(MYSQL_LIBS)

config_example_DATA = hatohol-config.dat.example
config_exampledir = $(pkgdatadir)

hatohol_def_src_file_generator_SOURCES = hatohol-def-src-file-generator.cc

hatohol_pymodules = \
  hatohol/__init__.py \
  hatohol/hatohol_def.py \
  hatohol/HatoholActionCreator.py \
  setup.py

# To make hahol_def.py on the build.
noinst_SCRIPTS = $(hatohol_pymodules)

hatohol/hatohol_def.py: hatohol-def-src-file-generator
	./hatohol-def-src-file-generator py > $@

install-data-hook:
	python setup.py install --root=$(DESTDIR)

EXTRA_DIST = $(config_example_DATA) $(hatohol_pymodules)
