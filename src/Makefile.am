sbin_PROGRAMS = asura
asura_SOURCES = \
	main.cc FaceBase.cc FaceMySQL.cc FaceMySQLWorker.cc \
	AsuraThreadBase.cc ArmBase.cc ArmZabbixAPI.cc

AM_CXXFLAGS = \
	$(MLPL_CFLAGS) $(GLIB_CFLAGS) $(GIO_CFLAGS) $(LIBSOUP_CFLAGS) -Wall -g

AM_LDFLAGS = \
	$(MLPL_LIBS) $(GLIB_LIBS) $(GIO_LIBS) $(LIBSOUP_LIBS)
