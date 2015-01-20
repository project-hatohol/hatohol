SUBDIRS = server client data data/init.d data/systemd
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST = autogen.sh README.md hatohol.spec \
             COPYING.GPLv3 COPYING.LGPLv3

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

# Add a subset of "distcheck" since it's too strict for us.
hatoholdistcheck: dist
	bzip2 -dc $(distdir).tar.bz2 | $(am__untar)
	mkdir $(distdir)/_inst
	dc_install_base=`$(am__cd) $(distdir)/_inst && pwd | sed -e 's,^[^:\\/]:[\\/],/,'` \
	  && am__cwd=`pwd` \
	  && $(am__cd) $(distdir) \
	  && ./configure --prefix="$$dc_install_base" \
	    $(AM_DISTCHECK_CONFIGURE_FLAGS) \
	    $(DISTCHECK_CONFIGURE_FLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) install \
	  && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	  && $(MAKE) $(AM_MAKEFLAGS) uninstall \
	  && $(MAKE) $(AM_MAKEFLAGS) distuninstallcheck_dir="$$dc_install_base" \
	        distuninstallcheck \
	  && cd "$$am__cwd" \
	  || exit 1
	$(am__post_remove_distdir)
