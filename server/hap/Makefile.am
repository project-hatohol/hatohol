sbin_PROGRAMS = hatohol-arm-plugin-zabbix
hatohol_arm_plugin_zabbix_SOURCES = hatohol-arm-plugin-zabbix.cc
# TODO: We divide libhatohol into the server specifiled part and not.
#       Then we only link the lattter library.
hatohol_arm_plugin_zabbix_LDADD = libhapprocess.la ../src/libhatohol.la

lib_LTLIBRARIES = libhapprocess.la
libhapprocess_la_SOURCES = \
	HapProcess.cc HapProcess.h

AM_CXXFLAGS = \
	-I ../src $(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS)

AM_LDFLAGS = \
	$(MLPL_LIBS) $(GLIB_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS)
