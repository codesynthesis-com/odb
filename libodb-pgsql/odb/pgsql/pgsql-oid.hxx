// file      : odb/pgsql/pgsql-oid.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Generated file of OIDs extracted from the PostgreSQL 8.4.8 source file
// src/include/catalog/pg_type.h
//

#ifndef ODB_PGSQL_PGSQL_OID_HXX
#define ODB_PGSQL_PGSQL_OID_HXX

#include <odb/pre.hxx>

#include <odb/pgsql/version.hxx>

namespace odb
{
  namespace pgsql
  {
    enum
    {
      bool_oid = 16,
      int2_oid = 21,
      int4_oid = 23,
      int8_oid = 20,

      numeric_oid = 1700,

      float4_oid = 700,
      float8_oid = 701,

      date_oid = 1082,
      time_oid = 1083,
      timestamp_oid = 1114,

      text_oid = 25,
      bytea_oid = 17,
      bit_oid = 1560,
      varbit_oid = 1562,

      void_oid = 2278,

      uuid_oid = 2950
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_PGSQL_OID_HXX
