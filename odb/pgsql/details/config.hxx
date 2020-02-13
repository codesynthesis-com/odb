// file      : odb/pgsql/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DETAILS_CONFIG_HXX
#define ODB_PGSQL_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  error libodb-pgsql header included in odb-compiled header
#elif !defined(LIBODB_PGSQL_BUILD2)
#  ifdef _MSC_VER
#    include <odb/pgsql/details/config-vc.h>
#  else
#    include <odb/pgsql/details/config.h>
#  endif
#endif

// no post

#endif // ODB_PGSQL_DETAILS_CONFIG_HXX
