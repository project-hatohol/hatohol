sbin_PROGRAMS = asura
asura_SOURCES = main.cc

lib_LTLIBRARIES = libasura.la
libasura_la_SOURCES = \
	FaceBase.cc FaceMySQL.cc FaceMySQLWorker.cc \
	SQLProcessorFactory.cc SQLProcessor.cc SQLProcessorZabbix.cc \
	JsonBuilderAgent.cc JsonParserAgent.cc \
	AsuraException.cc AsuraThreadBase.cc ArmController.cc \
	ArmBase.cc ArmZabbixAPI.cc

AM_CXXFLAGS = -Wall -g \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GIO_CFLAGS) \
	$(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS)

AM_LDFLAGS = \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GIO_LIBS) \
	$(LIBSOUP_LIBS) $(JSON_GLIB_LIBS)

asura_LDADD = -lasura
