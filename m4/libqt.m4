dnl file      : m4/libqt.m4
dnl copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl
dnl LIBQTCORE_COMPILE()
dnl
AC_DEFUN([LIBQTCORE_COMPILE], [
CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
// See libodb-qt/odb/qt/details/config.hxx for more information on
// what's going on here.
//
#  if defined(__ELF__) && !defined(__PIC__) && !defined(__PIE__)
#    include <QtCore/qconfig.h> // QT_REDUCE_RELOCATIONS
#    ifdef QT_REDUCE_RELOCATIONS
#      define __PIE__
#    endif
#  endif
#include <string>
#include <QtCore/QString>

int
main ()
{
  QString qs ("test");
  std::string ss (qs.toStdString ());
  return ss.size () != 0;
}
])],
[libqt_found=yes])
])dnl
dnl
dnl LIBQTCORE([ACTION-IF-FOUND[,ACTION-IF-NOT-FOUND]])
dnl
dnl
AC_DEFUN([LIBQTCORE], [
libqt_found=no

# QtCore libraries and pkg-config names in the order we should try
# them.
#
libqt_lib_names="Qt5Core QtCore5 QtCore Qt4Core QtCore4"
libqt_pkg_names="Qt5Core QtCore"

AC_PATH_PROG([pkg_config],[pkg-config])

AC_MSG_CHECKING([for QtCore])

# First check for QtCore using default CPPFLAGS/LDFLAGS/LIBS. This
# test allows the user to override the QtCore library name (e.g.,
# QtCored4, Qt5Core) via the LIBS variable.
#
LIBQTCORE_COMPILE

# Then check for QtCore using default CPPFLAGS/LDFLAGS.
#
if test x"$libqt_found" = xno; then
  for lib in $libqt_lib_names; do
    save_LIBS="$LIBS"
    LIBS="-l$lib $LIBS"

    LIBQTCORE_COMPILE

    if test x"$libqt_found" = xno; then
      LIBS="$save_LIBS"
    else
      break
    fi
  done
fi

# Try framework in case we are on Mac OS X.
#
if test x"$libqt_found" = xno; then
  save_LIBS="$LIBS"
  LIBS="-framework QtCore $LIBS"

  LIBQTCORE_COMPILE

  if test x"$libqt_found" = xno; then
    LIBS="$save_LIBS"
  fi
fi

# If default CPPFLAGS/LDFLAGS didn't work, try to discover
# them using pkg-config.
#
if test x"$libqt_found" = xno; then
  if test x"$pkg_config" != x; then
    for pkg in $libqt_pkg_names; do
      if $pkg_config --exists $pkg; then
        save_CPPFLAGS="$CPPFLAGS"
        save_LIBS="$LIBS"

        CPPFLAGS=`$pkg_config --cflags $pkg`
        LIBS=`$pkg_config --libs $pkg`

        CPPFLAGS="$CPPFLAGS $save_CPPFLAGS"
        LIBS="$LIBS $save_LIBS"

        LIBQTCORE_COMPILE

        if test x"$libqt_found" = xno; then
          CPPFLAGS="$save_CPPFLAGS"
          LIBS="$save_LIBS"
        else
          break
        fi
      fi
    done
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
