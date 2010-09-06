dnl file      : m4/static-lib.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl STATIC_LIB(MACRO, DESCRIPTION)
dnl
dnl Define MACRO if we are on MinGW and are only building static library.
dnl
AC_DEFUN([STATIC_LIB],
[
if test x$enable_shared = xno; then
  case $host_os in
    mingw*)
      AC_DEFINE([$1], [1], [$2])
      ;;
  esac
fi
])dnl
