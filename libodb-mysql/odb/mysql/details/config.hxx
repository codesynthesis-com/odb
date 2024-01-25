// file      : odb/mysql/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_DETAILS_CONFIG_HXX
#define ODB_MYSQL_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  error libodb-mysql header included in odb-compiled header
#elif !defined(LIBODB_MYSQL_BUILD2)
#  ifdef _MSC_VER
#    include <odb/mysql/details/config-vc.h>
#  else
#    include <odb/mysql/details/config.h>
#  endif
#endif

// LIBODB_MYSQL_BUILD2 macro can be defined either by the buildfile or by the
// included odb/mysql/details/config*.h (see above).
//
#ifdef LIBODB_MYSQL_BUILD2
#  ifdef LIBODB_MYSQL_INCLUDE_SHORT
#    error mysql headers must be included with mysql/ prefix
#  elif !defined(LIBODB_MYSQL_INCLUDE_LONG)
#    define LIBODB_MYSQL_INCLUDE_LONG 1
#  endif
#endif

// no post

#endif // ODB_MYSQL_DETAILS_CONFIG_HXX
