// file      : odb/mssql/mssql-fwd.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MSSQL_MSSQL_FWD_HXX
#define ODB_MSSQL_MSSQL_FWD_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

// Forward declaration for some of the types defined in sqltypes.h or
// sqlncli.h. This allows us to avoid having to include these files
// in public headers.
//
#ifdef _WIN32

// Keep consistent with Windows ODBC headers.
//

typedef long             SQLINTEGER;
typedef unsigned long    SQLUINTEGER;

#ifdef _WIN64
typedef __int64          SQLLEN;
typedef unsigned __int64 SQLULEN;
#else
#ifndef SQLLEN
typedef SQLINTEGER       SQLLEN;
typedef SQLUINTEGER      SQLULEN;
#endif
#endif

#else // _WIN32

// Keep consistent with unixODBC headers.
//

template <std::size_t sizeof_long>
struct odbc_types;

template <>
struct odbc_types<4>
{
  typedef long           integer;
  typedef unsigned long  uinteger;

  typedef integer        len;
  typedef uinteger       ulen;
};

template <>
struct odbc_types<8>
{
  typedef int            integer;
  typedef unsigned int   uinteger;

  typedef long           len;
  typedef unsigned long  ulen;
};

typedef odbc_types<sizeof (long)>::integer  SQLINTEGER;
typedef odbc_types<sizeof (long)>::uinteger SQLUINTEGER;

#ifndef SQLLEN
typedef odbc_types<sizeof (long)>::len  SQLLEN;
typedef odbc_types<sizeof (long)>::ulen SQLULEN;
#endif

#endif // _WIN32

typedef short            SQLSMALLINT;
typedef unsigned short   SQLUSMALLINT;

typedef SQLSMALLINT      SQLRETURN;

typedef void*            SQLHANDLE;
typedef SQLHANDLE        SQLHENV;
typedef SQLHANDLE        SQLHDBC;
typedef SQLHANDLE        SQLHSTMT;
typedef SQLHANDLE        SQLHDESC;

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
