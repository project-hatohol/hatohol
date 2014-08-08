SUBDIRS = server client data data/init.d
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST = autogen.sh README.md hatohol.spec

rpm:    dist hatohol.spec
	rpmbuild -ta $(distdir).tar.bz2

remove-merged-branches: remove-local-merged-branches
remove-merged-branches: remove-remote-merged-branches

remove-local-merged-branches:
	git branch --merged | \
	  grep -v '^* ' | \
	  grep -v ' master' | \
	  xargs -I BRANCH git branch --delete BRANCH

remove-remote-merged-branches:
	git branch --remote --merged | \
	  grep '^  origin/' | \
	  sed -e 's,^  origin/,,' | \
	  grep -v ' -> ' | \
	  grep -v '^master$$' | \
	  xargs -I BRANCH git push origin :BRANCH
