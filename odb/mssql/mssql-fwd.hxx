// file      : odb/mssql/mssql-fwd.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

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

#ifndef SQL_NULL_DATA
#  define SQL_NULL_DATA     (-1)
#  define SQL_DATA_AT_EXEC  (-2)
#  define SQL_NO_TOTAL      (-4)
#endif

// The following types are our own equivalents of ODBC and Native Client
// ODBC driver types. They are all PODs and should be layout-compatible
// with the original types, which means they can be used interchangeably.
//
namespace odb
{
  namespace mssql
  {
    // UCS-2 character type (SQLWCHAR).
    //
#ifdef _WIN32
    typedef wchar_t ucs2_char;
#else
    typedef unsigned short ucs2_char;
#endif

    // SQL_NUMERIC_STRUCT
    //
#ifndef SQL_MAX_NUMERIC_LEN
#define SQL_MAX_NUMERIC_LEN 16
#else
#  if SQL_MAX_NUMERIC_LEN != 16
#    error unexpected SQL_NUMERIC_STRUCT value
#  endif
#endif

    struct decimal
    {
      unsigned char precision;
      signed char scale;
      unsigned char sign; // 1 - positive, 0 - negative
      unsigned char val[SQL_MAX_NUMERIC_LEN];
    };

    // DBMONEY
    //
    struct money
    {
      // 8-byte signed integer containing value * 10,000.
      //
      int high;
      unsigned int low;
    };

    // DBMONEY4
    //
    struct smallmoney
    {
      int value; // 4-byte signed integer containing value * 10,000.
    };

    // SQL_DATE_STRUCT
    //
    struct date
    {
      SQLSMALLINT year;
      SQLUSMALLINT month;
      SQLUSMALLINT day;
    };

    // SQL_SS_TIME2_STRUCT
    //
#pragma pack(push,8)
    struct time
    {
      SQLUSMALLINT hour;
      SQLUSMALLINT minute;
      SQLUSMALLINT second;
      SQLUINTEGER fraction;
    };
#pragma pack(pop)

    // SQL_TIMESTAMP_STRUCT
    //
    struct datetime
    {
      SQLSMALLINT year;
      SQLUSMALLINT month;
      SQLUSMALLINT day;
      SQLUSMALLINT hour;
      SQLUSMALLINT minute;
      SQLUSMALLINT second;
      SQLUINTEGER fraction;
    };

    // SQL_SS_TIMESTAMPOFFSET_STRUCT
    //
#pragma pack(push,8)
    struct datetimeoffset
    {
      SQLSMALLINT year;
      SQLUSMALLINT month;
      SQLUSMALLINT day;
      SQLUSMALLINT hour;
      SQLUSMALLINT minute;
      SQLUSMALLINT second;
      SQLUINTEGER fraction;
      SQLSMALLINT timezone_hour;
      SQLSMALLINT timezone_minute;
    };
#pragma pack(pop)

    // SQLGUID
    //
    struct uniqueidentifier
    {
      unsigned int data1;
      unsigned short data2;
      unsigned short data3;
      unsigned char data4[8];
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_MSSQL_FWD_HXX
