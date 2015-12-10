SUBDIRS = hatohol tls

bin_PROGRAMS = hatohol-def-src-file-generator
dist_bin_SCRIPTS = hatohol-voyager \
                   hatohol-db-initiator \
                   hatohol-house-keeper \
                   hatohol-inspect-info-collector \
                   hatohol-server-type-util

actiondir = $(libexecdir)/$(PACKAGE)/action
dist_action_SCRIPTS = hatohol-actor-mail

MLPL_CFLAGS = -I $(top_srcdir)/server/mlpl/src
MLPL_LIBS = $(top_builddir)/server/mlpl/src/libmlpl.la

AM_CXXFLAGS = \
	$(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(MYSQL_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) \
	-I $(top_srcdir)/server/src \
	-I $(top_srcdir)/server/common

AM_LDFLAGS = \
	$(MLPL_LIBS) $(GLIB_LIBS) $(MYSQL_LIBS) \
	$(top_builddir)/server/src/libhatohol.la \
	$(top_builddir)/server/common/libhatohol-common.la

hatohol_def_src_file_generator_SOURCES = hatohol-def-src-file-generator.cc
hatohol_def_src_file_generator_LDADD = \
	$(MLPL_LIBS) \
	$(top_builddir)/server/src/libhatohol.la \
	$(top_builddir)/server/common/libhatohol-common.la

$(MLPL_LIBS):
	$(MAKE) -C $(top_builddir)/server/mlpl/src
$(top_builddir)/server/common/libhatohol-common.la:
	$(MAKE) -C $(top_builddir)/server/common
$(top_builddir)/server/src/libhatohol.la:
	$(MAKE) -C $(top_builddir)/server/src

# To make hahol_def.py on the build.
noinst_SCRIPTS = \
	setup.py \
	hatohol/hatohol_def.py \
	hatohol-init-user-generator

hatohol/hatohol_def.py: hatohol-def-src-file-generator
	./hatohol-def-src-file-generator py > $@

install-data-hook:
	python setup.py install --root=$(DESTDIR)

EXTRA_DIST = $(noinst_SCRIPTS)
