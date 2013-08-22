SUBDIRS = server client
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = autogen.sh README.md hatohol.spec

initscript_DATA = init/hatohol-debian init/hatohol-cent
initscriptdir = $(pkgdatadir)

rpm:    dist hatohol.spec
	rpmbuild -ta $(distdir).tar.bz2

