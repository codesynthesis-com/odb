// file      : odb/mysql/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_DETAILS_CONFIG_HXX
#define ODB_MYSQL_DETAILS_CONFIG_HXX

// NOTE: included in public interface (version.hxx.in).

// no pre

#ifdef ODB_COMPILER
#  error libodb-mysql header included in odb-compiled header
#endif

// @@ TMP: drop (along with LIBODB_MYSQL_THR_KEY_VISIBLE) after 2.5.0.
//
#ifdef LIBODB_MYSQL_INCLUDE_SHORT
#  error mysql headers must be included with mysql/ prefix
#elif !defined(LIBODB_MYSQL_INCLUDE_LONG)
#  define LIBODB_MYSQL_INCLUDE_LONG 1
#endif

// Define LIBODB_MYSQL_MARIADB macro if MariaDB is installed in the
// libmysqlclient compatibility mode.
//
#ifndef LIBODB_MYSQL_MARIADB

// Note that MariaDB issues the 'should not be included by clients' warning
// when <mysql/mysql_version.h> is included directly. Let's just suppress this
// warning.
//
// Also note that this approach doesn't work for GCC prior to the version 13.
// To suppress this warning for the older GCC versions the -Wno-cpp compiler
// option needs to be specified.
//
#  if defined(__clang__) || defined(__GNUC__)
#    pragma GCC diagnostic push

// Note that the older versions of Clang don't recognize the -Wcpp warning
// group. For them we use the synonym -W#warnings instead.
//
#    if defined(__clang__) && __clang_major__ < 6
#      pragma GCC diagnostic ignored "-W#warnings"
#    else
#      pragma GCC diagnostic ignored "-Wcpp"
#    endif
#  endif

#  include <mysql/mysql_version.h>

#  if defined(__clang__) || defined(__GNUC__)
#    pragma GCC diagnostic pop
#  endif

#  ifdef MARIADB_VERSION_ID
#    define LIBODB_MYSQL_MARIADB
#  endif
#endif

// no post

#endif // ODB_MYSQL_DETAILS_CONFIG_HXX
