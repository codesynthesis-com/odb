// file      : odb/mssql/mssql.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_MSSQL_HXX
#define ODB_MSSQL_MSSQL_HXX

#include <odb/pre.hxx>

// This file should always be included before mssql-fwd.hxx.
//
#ifdef ODB_MSSQL_MSSQL_FWD_HXX
#  error odb/mssql/mssql-fwd.hxx included before odb/mssql/mssql.hxx
#endif

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#include <sqlext.h>  // Standard ODBC.

//#define _SQLNCLI_ODBC_
//#include <sqlncli.h> // SQL Server Native Client driver specifics.

// Instead of having a dependency on <sqlncli.h> (which, BTW, is not
// currently available for the Linux version of the Native Client),
// we are going to provide the few definitions that we need ourselves.
//
#ifndef SQL_SS_LENGTH_UNLIMITED
#  define SQL_SS_LENGTH_UNLIMITED 0
#endif

#ifndef SQL_COPT_SS_BASE
#  define SQL_COPT_SS_BASE 1200
#endif

#ifndef SQL_COPT_SS_MARS_ENABLED
#  define SQL_COPT_SS_MARS_ENABLED (SQL_COPT_SS_BASE + 24)
#endif

#ifndef SQL_MARS_ENABLED_NO
#  define SQL_MARS_ENABLED_NO  0L
#  define SQL_MARS_ENABLED_YES 1L
#endif

#ifndef SQL_SS_TIME2
#  define SQL_SS_TIME2           (-154)
#  define SQL_SS_TIMESTAMPOFFSET (-155)
#endif

// unixODBC doesn't define SQL_PARAM_DATA_AVAILABLE even though it
// claims ODBC version 3.80.
//
#if ODBCVER >= 0x0380
#  ifndef SQL_PARAM_DATA_AVAILABLE
#    define SQL_PARAM_DATA_AVAILABLE 101
#  endif
#endif

#include <odb/post.hxx>

#endif // ODB_MSSQL_MSSQL_HXX
