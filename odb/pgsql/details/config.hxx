// file      : odb/pgsql/details/config.hxx
// copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DETAILS_CONFIG_HXX
#define ODB_PGSQL_DETAILS_CONFIG_HXX

// no pre

#ifdef _MSC_VER
#elif defined(ODB_COMPILER)
#  error libodb-pgsql header included in odb-compiled header
#else
#  include <odb/pgsql/details/config.h>
#endif

// no post

#endif // ODB_PGSQL_DETAILS_CONFIG_HXX
