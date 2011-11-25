# file      : configure.ac
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

AC_PREREQ(2.60)
AC_INIT([libodb-oracle], [__value__(version)], [odb-users@codesynthesis.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([odb/oracle/version.hxx])

AM_INIT_AUTOMAKE([-Wall -Werror foreign nostdinc subdir-objects dist-bzip2 dist-zip tar-ustar])

LT_INIT([win32-dll])

AC_CANONICAL_HOST

# Check for C++ compiler and use it to compile the tests.
#
AC_PROG_CXX
AC_LANG(C++)

# Create the libtool executable so that we can use it in further tests.
#
LT_OUTPUT

# Check for threads.
#
THREADS

# Check for OCI.
#
LIBOCI(
  [],
  [AC_MSG_ERROR([oci is not found; consider using --with-oci=DIR])])

# Check for libodb.
#
LIBODB([],[AC_MSG_ERROR([libodb is not found; consider using --with-libodb=DIR])])

# Define LIBODB_ORACLE_STATIC_LIB if we are build static library on certain
# platforms.
#
STATIC_LIB([LIBODB_ORACLE_STATIC_LIB], [Static library interface.])

# Allow the user to specify the pkgconfig directory.
#
PKGCONFIG

# Check if we should disable rpath.
#
DISABLE_RPATH

# Output.
#
AC_CONFIG_HEADERS([odb/oracle/config.h odb/oracle/details/config.h])
AC_CONFIG_FILES([__path__(config_files)])
AC_OUTPUT
