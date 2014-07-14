SUBDIRS = server client data/init.d
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST = autogen.sh README.md hatohol.spec

rpm:    dist hatohol.spec
	rpmbuild -ta $(distdir).tar.bz2

