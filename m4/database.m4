dnl file      : m4/database.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl DATABASE
dnl
AC_DEFUN([DATABASE], [
database=none

AC_MSG_CHECKING([for database to use])

AC_ARG_WITH(
  [database],
  [AC_HELP_STRING([--with-database=db],
                  [database to use for tests; valid values are: 'mysql'])],
  [case $withval in
     no | yes)
       AC_MSG_RESULT([])
       AC_MSG_ERROR([no database specified in the --with-database option])
       ;;
     mysql)
       database=mysql
       AC_DEFINE([DATABASE_MYSQL], [1], [Using MySQL.])
       ;;
     *)
       AC_MSG_RESULT([])
       AC_MSG_ERROR([unknown database $withval])
       ;;
   esac],
  [
   AC_MSG_RESULT([])
   AC_MSG_ERROR([no database specified with the --with-database option])
  ])

AC_MSG_RESULT([$database])
AC_SUBST([database])

AM_CONDITIONAL([DATABASE_MYSQL], [test x$database = xmysql])

])dnl
