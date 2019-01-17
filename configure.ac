# file      : configure.ac
# copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
# license   : GNU GPL v2; see accompanying LICENSE file

AC_PREREQ(2.60)
AC_INIT([odb-examples], [__value__(version)], [odb-users@codesynthesis.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([hello/driver.cxx])

AM_INIT_AUTOMAKE([-Wall -Werror foreign nostdinc subdir-objects dist-bzip2 dist-zip tar-ustar])
m4_equote()[m4_ifdef]m4_dquote()([AM_PROG_AR], [AM_PROG_AR]) # Required by automake 1.12.

LT_INIT

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

AM_CONDITIONAL([ODB_EXAMPLES_THREADS], [test x$threads != xnone])

# Check for C++11.
#
CXX11([HAVE_CXX11], [Compiling in the C++11 mode.])

# Check for the ODB compiler.
#
ODB_COMPILER([], [AC_MSG_ERROR([odb compiler is not found; consider setting ODB variable or using --with-odb=DIR])])

# Check for the ODB libs.
#
LIBODB([], [AC_MSG_ERROR([libodb is not found; consider using --with-libodb=DIR])])

# Check for TR1 <memory> availability (has to be after libodb).
#
TR1_MEMORY

# Check for boost.
#
odb_examples_boost=yes
LIBBOOST([], [odb_examples_boost=no])
LIBBOOST_SYSTEM
LIBBOOST_SMART_PTR([], [odb_examples_boost=no])
LIBBOOST_UNORDERED([], [odb_examples_boost=no])
LIBBOOST_DATE_TIME([], [odb_examples_boost=no])

# Check for libodb-boost.
#
LIBODB_BOOST([], [odb_examples_boost=no])

AM_CONDITIONAL([ODB_EXAMPLES_BOOST], [test x$odb_examples_boost != xno])

# Check for Qt.
#
odb_examples_qt=yes
LIBQTCORE([], [odb_examples_qt=no])

# Check for libodb-qt.
#
LIBODB_QT([], [odb_examples_qt=no])

AM_CONDITIONAL([ODB_EXAMPLES_QT], [test x$odb_examples_qt != xno])

# Check which database we are using.
#
DATABASE

case $database in
  mysql)
    LIBODB_MYSQL([], [AC_MSG_ERROR([libodb-mysql is not found; consider using --with-libodb-mysql=DIR])])
    MYSQL
    ;;
  sqlite)
    LIBODB_SQLITE([], [AC_MSG_ERROR([libodb-sqlite is not found; consider using --with-libodb-sqlite=DIR])])
    SQLITE
    ;;
  pgsql)
    LIBODB_PGSQL([], [AC_MSG_ERROR([libodb-pgsql is not found; consider using --with-libodb-pgsql=DIR])])
    PGSQL
    ;;
  oracle)
    LIBODB_ORACLE([], [AC_MSG_ERROR([libodb-oracle is not found; consider using --with-libodb-oracle=DIR])])
    ORACLE
    ;;
  mssql)
    LIBODB_MSSQL([], [AC_MSG_ERROR([libodb-mssql is not found; consider using --with-libodb-mssql=DIR])])
    MSSQL
    ;;
esac

# Output.
#
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([__path__(config_files)])
AC_CONFIG_COMMANDS([tester-mode], [chmod +x tester])
AC_OUTPUT
