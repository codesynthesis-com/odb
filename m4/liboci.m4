dnl file      : m4/oci.m4
dnl copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
dnl license   : GNU GPL v2; see accompanying LICENSE file
dnl
dnl LIBOCI([ACTION-IF-FOUND[,
dnl        ACTION-IF-NOT-FOUND]])
dnl
dnl
AC_DEFUN([LIBOCI], [
oci_found=no

AC_ARG_WITH(
  [oci],
  [AC_HELP_STRING([--with-oci=DIR],[oracle home or instant client directory])],
  [oci_dir=${withval}],
  [oci_dir=])

AC_MSG_CHECKING([for oci])

save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"

# If oci_dir was given, add the necessary preprocessor and linker flags.
#
if test x"$oci_dir" != x; then
  # Check whether oci_dir refers to Oracle home or an Instant Client
  # directory.
  #
  if test -d "$oci_dir/sdk/include"; then
    CPPFLAGS="$CPPFLAGS -I$oci_dir/sdk/include"
    LDFLAGS="$LDFLAGS -L$oci_dir"
    LIBS="-lclntsh $LIBS"

    CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#include <oci.h>

int
main ()
{
  OCIEnv* env (0);
  OCIEnvNlsCreate (&env, OCI_THREADED, 0, 0, 0, 0, 0, 0, 0, 0);
  OCIHandleFree (env, OCI_HTYPE_ENV);
  return 0;
}
])],
[
oci_found=yes
])

    if test x"$oci_found" = xno; then
      LIBS="-Wc,`ls $oci_dir/libclntsh.* 2>/dev/null` $save_LIBS"
    fi
  elif test -d "$oci_dir/rdbms/public"; then
    CPPFLAGS="$CPPFLAGS -I$oci_dir/rdbms/public"
    LDFLAGS="$LDFLAGS -L$oci_dir/lib"
    LIBS="-lclntsh $LIBS"
  fi
else
  LIBS="-lclntsh $LIBS"
fi

if test x"$oci_found" = xno; then

  CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#include <oci.h>

int
main ()
{
  OCIEnv* env (0);
  OCIEnvNlsCreate (&env, OCI_THREADED, 0, 0, 0, 0, 0, 0, 0, 0);
  OCIHandleFree (env, OCI_HTYPE_ENV);
  return 0;
}
])],
[
oci_found=yes
])

fi

if test x"$oci_found" = xno; then

  CPPFLAGS="$save_CPPFLAGS"
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  # Try using ORACLE_HOME if it exists.
  #
  if test x"$ORACLE_HOME" != x; then
    CPPFLAGS="$CPPFLAGS -I$ORACLE_HOME/rdbms/public"
    LDFLAGS="$LDFLAGS -L$ORACLE_HOME/lib"
    LIBS="-lclntsh $LIBS"

    CXX_LIBTOOL_LINK_IFELSE([
AC_LANG_SOURCE([
#include <oci.h>

int
main ()
{
  OCIEnv* env (0);
  OCIEnvNlsCreate (&env, OCI_THREADED, 0, 0, 0, 0, 0, 0, 0, 0);
  OCIHandleFree (env, OCI_HTYPE_ENV);
  return 0;
}
])],
[
oci_found=yes
])

    if test x"$oci_found" = xno; then
      CPPFLAGS="$save_CPPFLAGS"
      LDFLAGS="$save_LDFLAGS"
      LIBS="$save_LIBS"
    fi

  fi
fi

if test x"$oci_found" = xyes; then
  AC_MSG_RESULT([yes])
  $1
else
  LIBS="$save_LIBS"
  AC_MSG_RESULT([no])
  $2
fi
])dnl
