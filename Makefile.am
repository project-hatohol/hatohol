SUBDIRS = server
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = autogen.sh README.md hatohol.spec

rpm:    dist empl.spec
	rpmbuild -ta $(distdir).tar.gz

