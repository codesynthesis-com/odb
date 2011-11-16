// file      : odb/mssql/mssql-fwd.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MSSQL_MSSQL_FWD_HXX
#define ODB_MSSQL_MSSQL_FWD_HXX

#include <odb/pre.hxx>

// Forward declaration for some of the types defined in sqltypes.h or
// sqlncli.h. This allows us to avoid having to include these files
// in public headers.
//
#ifdef _WIN32
typedef long          SQLINTEGER;
typedef unsigned long SQLUINTEGER;
#endif

#include <odb/post.hxx>

#endif // ODB_MSSQL_MSSQL_FWD_HXX
