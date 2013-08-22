SUBDIRS = server client
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = autogen.sh README.md hatohol.spec

initscript_debian_DATA = init/hatohol-debian
initscript_debiandir = $(pkgdatadir)
initscript_cent_DATA = init/hatohol-cent
initscript_centdir = $(pkgdatadir)

rpm:    dist hatohol.spec
	rpmbuild -ta $(distdir).tar.bz2

