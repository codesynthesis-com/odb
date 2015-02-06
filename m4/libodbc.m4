dnl file      : m4/libodbc.m4
dnl copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl LIBODBC([ACTION-IF-FOUND[,ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([LIBODBC], [
libodbc_found=no

AC_MSG_CHECKING([for libodbc])

save_LIBS="$LIBS"

case $host_os in
  mingw*)
    LIBS="-lodbc32 $LIBS"
    ;;
  *)
    LIBS="-lodbc $LIBS"
    ;;
esac

CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#include <sqlext.h>

int
main ()
{
  SQLHENV env;
  SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

  SQLHDBC conn;
  SQLAllocHandle (SQL_HANDLE_DBC, env, &conn);
  SQLDriverConnectA (conn,
                     0,
                     (SQLCHAR*) "example.com",
                     SQL_NTS,
                     0,
                     0,
                     0,
                     SQL_DRIVER_NOPROMPT);

  SQLHSTMT stmt;
  SQLAllocHandle (SQL_HANDLE_STMT, conn, &stmt);
  SQLPrepareA (stmt, (SQLCHAR*) "SELECT 1", SQL_NTS);
}
])],
[
libodbc_found=yes
])

if test x"$libodbc_found" = xno; then
  LIBS="$save_LIBS"
fi

if test x"$libodbc_found" = xyes; then
  AC_MSG_RESULT([yes])
  $1
else
  AC_MSG_RESULT([no])
  $2
fi
])dnl
