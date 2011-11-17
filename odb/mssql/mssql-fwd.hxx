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
typedef long           SQLINTEGER;
typedef unsigned long  SQLUINTEGER;
#endif

typedef short          SQLSMALLINT;
typedef unsigned short SQLUSMALLINT;

typedef void*          SQLHANDLE;
typedef SQLHANDLE      SQLHENV;
typedef SQLHANDLE      SQLHDBC;
typedef SQLHANDLE      SQLHSTMT;
typedef SQLHANDLE      SQLHDESC;

// If you get a redefinition error or warning for one of these macros,
// then that means you included this header (or one that includes it),
// before <sql.h> or <sqlext.h>. As a general rule, include <sql.h> or
// <sqlext.h> before any of the ODB headers.
//
#ifndef SQL_HANDLE_ENV
#  define SQL_HANDLE_ENV  1
#  define SQL_HANDLE_DBC  2
#  define SQL_HANDLE_STMT 3
#  define SQL_HANDLE_DESC 4
#endif

#include <odb/post.hxx>

#endif // ODB_MSSQL_MSSQL_FWD_HXX
