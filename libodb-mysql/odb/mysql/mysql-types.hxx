// file      : odb/mysql/mysql-types.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_MYSQL_TYPES_HXX
#define ODB_MYSQL_MYSQL_TYPES_HXX

#include <odb/mysql/details/config.hxx>

#include <odb/mysql/version.hxx>

// Starting with 8.0.1 instead of my_bool MySQL uses bool directly. We keep
// using the alias for compatibility with previous versions.
//
#if !defined(LIBODB_MYSQL_MARIADB) && MYSQL_VERSION_ID >= 80001
typedef bool my_bool;
#else
typedef char my_bool;
#endif

// Starting with 8.0.11 MySQL renamed st_mysql_bind to MYSQL_BIND.
//
#if !defined(LIBODB_MYSQL_MARIADB) && MYSQL_VERSION_ID >= 80011
struct MYSQL_BIND;
#else
typedef struct st_mysql_bind MYSQL_BIND;
#endif

// MariaDB defines time types directly in mysql.h. Note that MariaDB is only
// supported by the build2 build so we include the header as <mysql/mysql.h>
// unconditionally.
//
#ifdef LIBODB_MYSQL_MARIADB
#  include <mysql/mysql.h>
#else
#  ifdef LIBODB_MYSQL_INCLUDE_SHORT
#    include <mysql_time.h>
#  else
#    include <mysql/mysql_time.h>
#  endif
#endif

#endif // ODB_MYSQL_MYSQL_TYPES_HXX
