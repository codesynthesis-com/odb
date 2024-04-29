// file      : odb/mysql/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_DETAILS_CONFIG_HXX
#define ODB_MYSQL_DETAILS_CONFIG_HXX

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

// no post

#endif // ODB_MYSQL_DETAILS_CONFIG_HXX
