// file      : odb/mssql/details/config.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_DETAILS_CONFIG_HXX
#define ODB_MSSQL_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  error libodb-mssql header included in odb-compiled header
#elif !defined(LIBODB_MSSQL_BUILD2)
#  ifdef _MSC_VER
#    include <odb/mssql/details/config-vc.h>
#  else
#    include <odb/mssql/details/config.h>
#  endif
#endif

// no post

#endif // ODB_MSSQL_DETAILS_CONFIG_HXX
