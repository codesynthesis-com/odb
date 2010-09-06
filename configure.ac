# file      : configure.ac
# author    : Boris Kolpackov <boris@codesynthesis.com>
# copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

AC_PREREQ(2.60)
AC_INIT([odb-tests], [__value__(version)], [odb-users@codesynthesis.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([tracer/template/driver.cxx])

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

AM_CONDITIONAL([ODB_TESTS_THREADS], [test x$threads != xnone])

# Check for the ODB libs.
#
LIBODB([], [AC_MSG_ERROR([libodb is not found; consider using --with-libodb=DIR])])
LIBODB_TRACER([], [AC_MSG_ERROR([libodb-tracer is not found; consider using --with-libodb-tracer=DIR])])

# Check which database we are using.
#
DATABASE

case $database in
  mysql)
    LIBODB_MYSQL([], [AC_MSG_ERROR([libodb-mysql is not found; consider using --with-libodb-mysql=DIR])])
    MYSQL
    ;;
esac

# Check for the ODB compiler.
#
ODB_COMPILER([], [AC_MSG_ERROR([odb compiler is not found; consider setting ODB variable or using --with-odb=DIR])])

# Check for diff.
#
DIFF_TOOL

# Define LIBCOMMON_STATIC_LIB if we are build static library on certain
# platforms.
#
STATIC_LIB([LIBCOMMON_STATIC_LIB], [Static library interface.])

# Output.
#
AC_CONFIG_HEADERS([config.h libcommon/config.h])
AC_CONFIG_FILES([__path__(config_files)])
AC_CONFIG_COMMANDS([tester-mode], [chmod +x tester])
AC_OUTPUT
