dnl file      : m4/libodb-qt.m4
dnl copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl LIBODB_QT([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl
AC_DEFUN([LIBODB_QT], [
libodb_qt_found=no

AC_ARG_WITH(
  [libodb-qt],
  [AC_HELP_STRING([--with-libodb-qt=DIR],[location of libodb-qt build directory])],
  [libodb_qt_dir=${withval}],
  [libodb_qt_dir=])

AC_MSG_CHECKING([for libodb-qt])

# If libodb_qt_dir was given, add the necessary preprocessor and
# linker flags.
#
if test x"$libodb_qt_dir" != x; then
  save_CPPFLAGS="$CPPFLAGS"
  save_LDFLAGS="$LDFLAGS"

  AS_SET_CATFILE([abs_libodb_qt_dir], [$ac_pwd], [$libodb_qt_dir])

  CPPFLAGS="$CPPFLAGS -I$abs_libodb_qt_dir"
  LDFLAGS="$LDFLAGS -L$abs_libodb_qt_dir/odb/qt"
fi

save_LIBS="$LIBS"
LIBS="-lodb-qt $LIBS"

CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#include <odb/qt/exception.hxx>

void
f ()
{
}

const char*
g ()
{
  try
  {
    f ();
  }
  catch (const odb::qt::exception& e)
  {
    return e.what ();
  }
  return 0;
}

int
main ()
{
  const char* m (g ());
  return m != 0;
}
])],
[libodb_qt_found=yes])

if test x"$libodb_qt_found" = xno; then
  LIBS="$save_LIBS"

  if test x"$libodb_qt_dir" != x; then
    CPPFLAGS="$save_CPPFLAGS"
    LDFLAGS="$save_LDFLAGS"
  fi
fi

if test x"$libodb_qt_found" = xyes; then
  AC_MSG_RESULT([yes])
  $1
else
  AC_MSG_RESULT([no])
  $2
fi
])dnl
