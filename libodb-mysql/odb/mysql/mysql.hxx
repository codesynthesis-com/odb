// file      : odb/mysql/mysql.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_MYSQL_HXX
#define ODB_MYSQL_MYSQL_HXX

#include <odb/mysql/details/config.hxx>

#include <odb/mysql/mysql-types.hxx>

#ifdef _WIN32
#  ifndef NOMINMAX // No min and max macros.
#    define NOMINMAX
#    include <winsock2.h>
#    undef NOMINMAX
#  else
#    include <winsock2.h>
#  endif
#endif

#ifdef LIBODB_MYSQL_INCLUDE_SHORT
#  include <mysql.h>
#  include <errmsg.h>
#  include <mysqld_error.h>
#else
#  include <mysql/mysql.h>
#  include <mysql/errmsg.h>
#  include <mysql/mysqld_error.h>
#endif

#endif // ODB_MYSQL_MYSQL_HXX
