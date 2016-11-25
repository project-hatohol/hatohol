AC_PREREQ(2.63)

AC_INIT(hatohol,  m4_esyscmd([./version-generator.sh]))
AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-bzip2 tar-pax])

AC_CONFIG_HEADER(config.h)

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])

if test x"$prefix" = x"NONE"; then
    prefix=$ac_default_prefix
fi
AC_DEFINE_UNQUOTED(PREFIX, "$prefix", "Prefix for install directory")

if test x"$sysconfdir" = x"NONE"; then
    sysconfdir=$ac_default_sysconfdir
fi
expanded_sysconfdir="`eval echo $sysconfdir`"
AC_DEFINE_UNQUOTED(SYSCONFDIR, "$expanded_sysconfdir", "Data directory.")
AC_SUBST(expanded_sysconfdir)

if test x"$localstatedir" = x"NONE"; then
    runstatedir=$ac_default_localstatedir
fi
expanded_localstatedir="`eval echo $localstatedir`"
AC_DEFINE_UNQUOTED(LOCALSTATEDIR, "$expanded_localstatedir", "Local state directory.")

expanded_pkglocaldatadir="$expanded_localstatedir/lib/$PACKAGE"
AC_SUBST(expanded_pkglocaldatadir)

if test x"$exec_prefix" = x"NONE"; then
    exec_prefix=$prefix
fi
AC_DEFINE_UNQUOTED(LIBEXECDIR, "`eval echo $libexecdir`", "Prefix for libexec directory")

dnl **************************************************************
dnl Optional C++ compiler flags
dnl **************************************************************
OPT_CXXFLAGS="-Wall -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS"

enable_debug_default=no
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
                  [Enable debug build (default is $enable_debug_default)])],
  [],
  [enable_debug=$enable_debug_default])

if test "$enable_debug" = "yes"; then
  CXXFLAGS="$CXXFLAGS -O0 -g3"
else
  CXXFLAGS="$CXXFLAGS -O2 -g3"
fi

dnl **************************************************************
dnl Checks for GLib
dnl **************************************************************
GLIB_REQUIRED=2.22
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
PKG_CHECK_MODULES(GIO, gio-2.0 >= $GLIB_REQUIRED)

GTHREAD_REQUIRED=2.22
PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= $GTHREAD_REQUIRED)

dnl **************************************************************
dnl Checks for libsoup
dnl **************************************************************
LIBSOUP_REQUIRED=2.22
PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4 >= $LIBSOUP_REQUIRED)

dnl **************************************************************
dnl Checks for libjson-glib
dnl **************************************************************
JSON_GLIB_REQUIRED=0.12.0
PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0 >= $JSON_GLIB_REQUIRED)

dnl **************************************************************
dnl Checks for SQLite3
dnl **************************************************************
SQLITE3_REQUIRED=3.6.0
PKG_CHECK_MODULES(SQLITE3, sqlite3 >= $SQLITE3_REQUIRED)

dnl **************************************************************
dnl Checks for uuid
dnl **************************************************************
UUID_REQUIRED=2.17
PKG_CHECK_MODULES(UUID, uuid >= $UUID_REQUIRED)

dnl **************************************************************
dnl Checks for librabbitmq
dnl **************************************************************
LIBRABBITMQ_REQUIRED=0.4.1
PKG_CHECK_MODULES(LIBRABBITMQ, librabbitmq >= $LIBRABBITMQ_REQUIRED)

dnl **************************************************************
dnl Checks for libdl
dnl **************************************************************

DL_FUNCTIONS="[dlopen, dlclose, dlerror, dlsym]"
AC_CHECK_LIB([dl], $DL_FUNCTIONS, [],
  [AC_MSG_ERROR([Could not found '$DL_FUNCTIONS' in dlfcn.h.])])

dnl **************************************************************
dnl Checks for MySQL
dnl **************************************************************
AC_CHECK_PROGS(MYSQL_CONFIG, mysql_config)

MYSQL_CFLAGS=`$MYSQL_CONFIG --cflags`
MYSQL_LIBS=`$MYSQL_CONFIG --libs`

hatohol_save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $MYSQL_CFLAGS"
AC_TRY_COMPILE([#include <mysql_version.h>],
  [int version = MYSQL_VERSION_ID;], [],
  [AC_MSG_ERROR([You don't have mysql_version.h. Please install development package of MySQL.])])
CFLAGS=$hatohol_save_CFLAGS

AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)


dnl **************************************************************
dnl Check libraries for C++
dnl **************************************************************
AC_LANG_PUSH([C++])

dnl **************************************************************
dnl Flag setting for MLPL
dnl **************************************************************
MLPL_CFLAGS=-I\${abs_top_srcdir}/server/mlpl/src
MLPL_LIBS=\${top_builddir}/server/mlpl/src/libmlpl.la

AC_SUBST(MLPL_CFLAGS)
AC_SUBST(MLPL_LIBS)

AC_LANG_POP([C++])


dnl **************************************************************
dnl Checks for gettext
dnl **************************************************************
AC_PATH_PROG(MSGFMT, msgfmt, msgfmt-not-found)
if test x"$MSGFMT" = x"msgfmt-not-found"; then
  AC_MSG_ERROR([You don't have msgfmt command. Please install gettext tools.])
fi
AC_SUBST(MSGFMT)


dnl **************************************************************
dnl Checks for Cutter
dnl **************************************************************
CUTTER_REQUIRED=1.2.3
have_cutter=no

m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER(>= $CUTTER_REQUIRED)
],
[ac_cv_use_cutter="no"])

if test "$ac_cv_use_cutter" != "no"; then
  m4_ifdef([AC_CHECK_CPPCUTTER], [
  AC_CHECK_CPPCUTTER(>= $CUTTER_REQUIRED)
  ],
  [ac_cv_use_cutter="no"])
fi

AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])
if test "$ac_cv_use_cutter" != "no"; then
  AC_CHECK_PROG([have_sqlite3], [sqlite3], [yes], [no])
  if test "$have_sqlite3" = "no"; then
    AC_MSG_ERROR([You don't have sqlite3 command. It's required to generate test data.])
  fi
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])
  have_cutter=yes
fi

m4_ifdef([AC_CHECK_COVERAGE], [AC_CHECK_COVERAGE])

if test "$cutter_enable_coverage" = "yes" -a -z "$COVERAGE_LIBS"; then
  # old cutter doesn't have it
  COVERAGE_LIBS="-lgcov"
fi
AC_SUBST(COVERAGE_LIBS)

dnl **************************************************************
dnl enable C++11
dnl **************************************************************
OPT_CXXFLAGS+=" -std=c++11"
AC_SUBST(OPT_CXXFLAGS)

dnl **************************************************************
dnl Mocha, expect.js, and npm (indirectly node.js)
dnl **************************************************************
have_js_test_tools=yes
NODE_MODULES_PATH=$(npm bin)
AC_CHECK_PROG([have_npm], [npm], [yes], [no])
AC_CHECK_PROG([have_mocha], [mocha], [yes], [no], [$NODE_MODULES_PATH])
have_expect_js=no
if test "$have_npm" = "yes"; then
  expect_js_ver=`npm ls | grep expect.js | awk '{ print $2 }' FS=@`
  if test x"$expect_js_ver" != x; then
    have_expect_js=yes
  fi
  mocha_ver=`npm ls | grep mocha | awk '{ print $2 }' FS=@`
fi

if test "$have_mocha" = "no"; then
  have_js_test_tools=no
fi
if test "$have_npm" = "no"; then
  have_js_test_tools=no
fi
if test "$have_expect_js" = "no"; then
  have_js_test_tools=no
fi

AM_CONDITIONAL([WITH_JS_TEST], [test "$have_js_test_tools" = "yes"])

dnl **************************************************************
dnl Results
dnl **************************************************************

AC_OUTPUT([
Makefile
hatohol.spec
client/static/js/hatohol_version.js
client/hatohol/autotools_vars.py
data/Makefile
data/init.d/Makefile
data/init.d/debian/hatohol.debian
data/init.d/debian/Makefile
data/init.d/centos/hatohol.centos
data/init.d/centos/Makefile
data/systemd/Makefile
data/systemd/generate_service_file.sh
data/test/Makefile
server/Makefile
server/mlpl/Makefile
server/mlpl/src/Makefile
server/mlpl/test/Makefile
server/mlpl/mlpl.pc
server/common/Makefile
server/src/Makefile
server/log/Makefile
server/hap2/Makefile
server/hap2/hap2.conf
server/hap2/hap2-control-functions.sh
server/hap2/hatohol/Makefile
server/hap2/hatohol/autotools_vars.py
server/data/Makefile
server/tools/Makefile
server/tools/hatohol-db-initiator
server/tools/hatohol-house-keeper
server/tools/hatohol/Makefile
server/tools/tls/Makefile
server/tools/tls/hatohol-ca-initialize
server/tools/tls/hatohol-ca-sign-client-certificate
server/tools/tls/hatohol-ca-sign-server-certificate
server/test/Makefile
server/test/fixtures/Makefile
server/benchmark/Makefile
client/Makefile
client/test/Makefile
])

echo
echo "Configure Result:"
echo
echo "  C Unit test (cutter) : ${have_cutter}"
echo "  JS Unit test         : ${have_js_test_tools}"
echo
