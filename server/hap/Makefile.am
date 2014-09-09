LDADD = libhapprocess.la ../common/libhatohol-common.la

sbin_PROGRAMS = hatohol-arm-plugin-zabbix hatohol-arm-plugin-ceilometer
hatohol_arm_plugin_zabbix_SOURCES = hatohol-arm-plugin-zabbix.cc
hatohol_arm_plugin_ceilometer_SOURCES = hatohol-arm-plugin-ceilometer.cc

lib_LTLIBRARIES = libhapprocess.la

libhapprocess_la_SOURCES = \
	HatoholArmPluginBase.cc HatoholArmPluginBase.h \
	HatoholArmPluginStandard.cc HatoholArmPluginStandard.h \
	HapZabbixAPI.cc HapZabbixAPI.h \
	HapProcess.cc HapProcess.h \
	HapProcessStandard.cc HapProcessStandard.h \
	HapProcessCeilometer.cc HapProcessCeilometer.h

AM_CXXFLAGS = \
	-I ../common -I ../src $(OPT_CXXFLAGS) \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS)

AM_LDFLAGS = \
	$(MLPL_LIBS) $(GLIB_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS)
