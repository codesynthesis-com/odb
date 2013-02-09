dnl file      : m4/libpq.m4
dnl copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl LIBPQ([ACTION-IF-FOUND[,
dnl       ACTION-IF-NOT-FOUND]])
dnl
dnl
AC_DEFUN([LIBPQ], [
libpq_found=no

AC_PATH_PROG([pg_config],[pg_config])

AC_MSG_CHECKING([for libpq])

# Check for libpq using default CPPFLAGS/LDFLAGS.
#
save_LIBS="$LIBS"
LIBS="-lpq $LIBS"

CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#include <libpq-fe.h>

int
main ()
{
  PGconn* conn = PQconnectdb ("");
  char* dbname =  PQdb (conn);
  PQfinish (conn);
  return 0;
}
])],
[
libpq_found=yes
])

if test x"$libpq_found" = xno; then

  # If default CPPFLAGS/LDFLAGS didn't work, try to discover
  # them using pg_config.
  #
  if test x"$libpq_found" = xno; then

    if test x"$pg_config" != x; then
      save_CPPFLAGS="$CPPFLAGS"

      CPPFLAGS=-I`$pg_config --includedir`
      CPPFLAGS="$CPPFLAGS $save_CPPFLAGS"

      CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#include <libpq-fe.h>

int
main ()
{
  PGconn* conn = PQconnectdb ("");
  char* dbname =  PQdb (conn);
  PQfinish (conn);
  return 0;
}
])],
[
libpq_found=yes
])

     if test x"$libpq_found" = xno; then
       CPPFLAGS="$save_CPPFLAGS"
     fi

    fi
  fi
fi

if test x"$libpq_found" = xyes; then
  AC_MSG_RESULT([yes])
  $1
else
  LIBS="$save_LIBS"
  AC_MSG_RESULT([no])
  $2
fi
])dnl
