dnl file      : m4/pkgconfig.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
AC_DEFUN([PKGCONFIG],[
AC_ARG_WITH(
  [pkgconfigdir],
  [AC_HELP_STRING([--with-pkgconfigdir=DIR],[location of pkgconfig dir (default is libdir/pkgconfig)])],
  [pkgconfigdir=${withval}],
  [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])
])dnl
