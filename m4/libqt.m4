dnl file      : m4/libqt.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl LIBQTCORE([ACTION-IF-FOUND[,ACTION-IF-NOT-FOUND]])
dnl
dnl
AC_DEFUN([LIBQTCORE], [
libqt_found=no

AC_PATH_PROG([pkg_config],[pkg-config])

AC_MSG_CHECKING([for QtCore])

# First check for QtCore using default CPPFLAGS/LDFLAGS.
#
save_LIBS="$LIBS"
LIBS="-lQtCore $LIBS"

CXX_LIBTOOL_LINK_IFELSE(
AC_LANG_SOURCE([[
#include <string>
#include <QString>

int
main ()
{
  QString qs ("test");
  std::string ss (qs.toStdString ());
  return ss.size () != 0;
}
]]),
[
libqt_found=yes
])

# If default CPPFLAGS/LDFLAGS didn't work, try to discover
# them using pkg-config.
#
if test x"$libqt_found" = xno; then
  LIBS="$save_LIBS"

  if test x"$pkg_config" != x; then
    if $pkg_config --exists QtCore; then
      save_CPPFLAGS="$CPPFLAGS"
      save_LIBS="$LIBS"

      CPPFLAGS=`$pkg_config --cflags QtCore`
      LIBS=`$pkg_config --libs QtCore`

      CPPFLAGS="$CPPFLAGS $save_CPPFLAGS"
      LIBS="$LIBS $save_LIBS"

      CXX_LIBTOOL_LINK_IFELSE(
AC_LANG_SOURCE([[
#include <string>
#include <QString>

int
main ()
{
  QString qs ("test");
  std::string ss (qs.toStdString ());
  return ss.size () != 0;
}
]]),
[
libqt_found=yes
])
     if test x"$libqt_found" = xno; then
       CPPFLAGS="$save_CPPFLAGS"
       LIBS="$save_LIBS"
     fi
    fi
  fi
fi

if test x"$libqt_found" = xyes; then
  AC_MSG_RESULT([yes])
  $1
else
  AC_MSG_RESULT([no])
  $2
fi
])dnl
