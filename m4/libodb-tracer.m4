dnl file      : m4/libodb-tracer.m4
dnl author    : Boris Kolpackov <boris@codesynthesis.com>
dnl copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl LIBODB_TRACER([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl
AC_DEFUN([LIBODB_TRACER], [
libodb_tracer_found=no

AC_ARG_WITH(
  [libodb-tracer],
  [AC_HELP_STRING([--with-libodb-tracer=DIR],[location of libodb-tracer build directory])],
  [libodb_tracer_dir=${withval}],
  [libodb_tracer_dir=])

AC_MSG_CHECKING([for libodb-tracer])

# If libodb_tracer_dir was given, add the necessary preprocessor and
# linker flags.
#
if test x"$libodb_tracer_dir" != x; then
  save_CPPFLAGS="$CPPFLAGS"
  save_LDFLAGS="$LDFLAGS"

  AS_SET_CATFILE([abs_libodb_tracer_dir], [$ac_pwd], [$libodb_tracer_dir])

  CPPFLAGS="$CPPFLAGS -I$abs_libodb_tracer_dir"
  LDFLAGS="$LDFLAGS -L$abs_libodb_tracer_dir/odb/tracer"
fi

save_LIBS="$LIBS"
LIBS="-lodb-tracer $LIBS"

CXX_LIBTOOL_LINK_IFELSE(
AC_LANG_SOURCE([[
#include <odb/tracer/exceptions.hxx>

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
  catch (const odb::tracer::database_exception& e)
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
]]),
[libodb_tracer_found=yes])

if test x"$libodb_tracer_found" = xno; then
  LIBS="$save_LIBS"

  if test x"$libodb_tracer_dir" != x; then
    CPPFLAGS="$save_CPPFLAGS"
    LDFLAGS="$save_LDFLAGS"
  fi
fi

if test x"$libodb_tracer_found" = xyes; then
  AC_MSG_RESULT([yes])
  $1
else
  AC_MSG_RESULT([no])
  $2
fi
])dnl
