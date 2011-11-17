// file      : odb/mssql/odbc.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_ODBC_HXX
#define ODB_MSSQL_ODBC_HXX

#include <odb/pre.hxx>

// This file should always be included before mssql-fwd.hxx.
//
#ifdef ODB_MSSQL_MSSQL_FWD_HXX
#  error odb/mssql/mssql-fwd.hxx included before odb/mssql/odbc.hxx
#endif

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#include <sqlext.h>  // Standard ODBC.

//#define _SQLNCLI_ODBC_
//#include <sqlncli.h> // SQL Server native client driver specifics.

#include <odb/post.hxx>

#endif // ODB_MSSQL_ODBC_HXX
