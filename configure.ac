AC_PREREQ(2.63)

AC_INIT(hatohol, 0.1)
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 tar-pax])

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

if test x"$exec_prefix" = x"NONE"; then
    exec_prefix=$prefix
fi
AC_DEFINE_UNQUOTED(LIBEXECDIR, "`eval echo $libexecdir`", "Prefix for libexec directory")

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
dnl Flag setting for MLPL
dnl **************************************************************
MLPL_CFLAGS=-I\${abs_top_srcdir}/server/mlpl/src
MLPL_LIBS=\${abs_top_srcdir}/server/mlpl/src/libmlpl.la

AC_SUBST(MLPL_CFLAGS)
AC_SUBST(MLPL_LIBS)

dnl **************************************************************
dnl Checks for Cutter
dnl **************************************************************
CUTTER_REQUIRED=1.2.2
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

dnl **************************************************************
dnl Results
dnl **************************************************************

AC_OUTPUT([
Makefile
hatohol.spec
data/init.d/Makefile
data/init.d/debian/hatohol.debian
data/init.d/debian/Makefile
data/init.d/centos/hatohol.centos
data/init.d/centos/Makefile
server/Makefile
server/mlpl/Makefile
server/mlpl/src/Makefile
server/mlpl/test/Makefile
server/mlpl/mlpl.pc
server/src/Makefile
server/tools/Makefile
server/test/Makefile
server/test/fixtures/Makefile
client/Makefile
])

echo
echo "Configure Result:"
echo
echo "  C Unit test (cutter) : ${have_cutter}"
echo
