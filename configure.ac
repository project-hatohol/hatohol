AC_PREREQ(2.63)

AC_INIT(asura, 0.0.1)
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 tar-pax])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])

dnl **************************************************************
dnl Checks for MLPL
dnl **************************************************************
MLPL_REQUIRED=0.0.1
PKG_CHECK_MODULES(MLPL, mlpl >= $MLPL_REQUIRED)

dnl **************************************************************
dnl Checks for GLib
dnl **************************************************************
GLIB_REQUIRED=2.22
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
PKG_CHECK_MODULES(GIO, gio-2.0 >= $GLIB_REQUIRED)

dnl **************************************************************
dnl Checks for libsoup
dnl **************************************************************
LIBSOUP_REQUIRED=2.22
PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4 >= $LIBSOUP_REQUIRED)

dnl **************************************************************
dnl Checks for libjson-glib
dnl **************************************************************
JSON_GLIB_REQUIRED=0.15.0
PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0 >= $JSON_GLIB_REQUIRED)

dnl **************************************************************
dnl Checks for Cutter
dnl **************************************************************
CUTTER_REQUIRED=1.1.7
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
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])
  have_cutter=yes
fi


dnl **************************************************************
dnl Results
dnl **************************************************************

AC_OUTPUT([
Makefile
src/Makefile
])

echo
echo "Configure Result:"
echo
echo "  C Unit test (cutter) : ${have_cutter}"
echo
