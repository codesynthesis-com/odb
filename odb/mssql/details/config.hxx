// file      : odb/mssql/details/config.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_DETAILS_CONFIG_HXX
#define ODB_MSSQL_DETAILS_CONFIG_HXX

// no pre

#ifdef _MSC_VER
#elif defined(ODB_COMPILER)
#  error libodb-mssql header included in odb-compiled header
#else
#  include <odb/mssql/details/config.h>
#endif

// no post

#endif // ODB_MSSQL_DETAILS_CONFIG_HXX
