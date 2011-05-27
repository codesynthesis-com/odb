// file      : odb/pgsql/pgsql-oid.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
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

    enum oid
    {
      bool_oid = 16,
      bytea_oid = 17,
      char_oid = 18,
      name_oid = 19,
      int8_oid = 20,
      int2_oid = 21,
      int2vector_oid = 22,
      int4_oid = 23,
      regproc_oid = 24,
      text_oid = 25,
      oid_oid = 26,
      tid_oid = 27,
      xid_oid = 28,
      cid_oid = 29,
      oidvector_oid = 30,

      pg_type_oid = 71,
      pg_attribute_oid = 75,
      pg_proc_oid = 81,
      pg_class_oid = 83,

      xml_oid = 142,
      _xml_oid = 143,

      smgr_oid = 210,

      point_oid = 600,
      lseg_oid = 601,
      path_oid = 602,
      box_oid = 603,
      polygon_oid = 604,
      line_oid = 628,
      _line_oid = 629,

      float4_oid = 700,
      float8_oid = 701,
      abstime_oid = 702,
      reltime_oid = 703,
      tinterval_oid = 704,
      unknown_oid = 705,
      circle_oid = 718,
      _circle_oid = 719,
      money_oid = 790,
      _money_oid = 791,

      macaddr_oid = 829,
      inet_oid = 869,
      cidr_oid = 650,

      _bool_oid = 1000,
      _bytea_oid = 1001,
      _char_oid = 1002,
      _name_oid = 1003,
      _int2_oid = 1005,
      _int2vector_oid = 1006,
      _int4_oid = 1007,
      int4array_oid = 1007,
      _regproc_oid = 1008,
      _text_oid = 1009,
      textarray_oid = 1009,
      _oid_oid = 1028,
      _tid_oid = 1010,
      _xid_oid = 1011,
      _cid_oid = 1012,
      _oidvector_oid = 1013,
      _bpchar_oid = 1014,
      _varchar_oid = 1015,
      _int8_oid = 1016,
      _point_oid = 1017,
      _lseg_oid = 1018,
      _path_oid = 1019,
      _box_oid = 1020,
      _float4_oid = 1021,
      float4array_oid = 1021,
      _float8_oid = 1022,
      _abstime_oid = 1023,
      _reltime_oid = 1024,
      _tinterval_oid = 1025,
      _polygon_oid = 1027,
      aclitem_oid = 1033,
      _aclitem_oid = 1034,
      _macaddr_oid = 1040,
      _inet_oid = 1041,
      _cidr_oid = 651,
      _cstring_oid = 1263,
      cstringarray_oid = 1263,
      bpchar_oid = 1042,
      varchar_oid = 1043,
      date_oid = 1082,
      time_oid = 1083,

      timestamp_oid = 1114,
      _timestamp_oid = 1115,
      _date_oid = 1182,
      _time_oid = 1183,
      timestamptz_oid = 1184,
      _timestamptz_oid = 1185,
      interval_oid = 1186,
      _interval_oid = 1187,

      _numeric_oid = 1231,
      timetz_oid = 1266,
      _timetz_oid = 1270,

      bit_oid = 1560,
      _bit_oid = 1561,
      varbit_oid = 1562,
      _varbit_oid = 1563,

      numeric_oid = 1700,
      refcursor_oid = 1790,

      _refcursor_oid = 2201,
      regprocedure_oid = 2202,
      regoper_oid = 2203,
      regoperator_oid = 2204,
      regclass_oid = 2205,
      regtype_oid = 2206,
      _regprocedure_oid = 2207,
      _regoper_oid = 2208,
      _regoperator_oid = 2209,
      _regclass_oid = 2210,
      _regtype_oid = 2211,

      uuid_oid = 2950,
      _uuid_oid = 2951,

      tsvector_oid = 3614,
      gtsvector_oid = 3642,
      tsquery_oid = 3615,
      regconfig_oid = 3734,
      regdictionary_oid = 3769,
      _tsvector_oid = 3643,
      _gtsvector_oid = 3644,
      _tsquery_oid = 3645,
      _regconfig_oid = 3735,
      _regdictionary_oid = 3770,
      txid_snapshot_oid = 2970,
      _txid_snapshot_oid = 2949,

      record_oid = 2249,
      _record_oid = 2287,
      cstring_oid = 2275,
      any_oid = 2276,
      anyarray_oid = 2277,
      void_oid = 2278,
      trigger_oid = 2279,
      language_handler_oid = 2280,
      internal_oid = 2281,
      opaque_oid = 2282,
      anyelement_oid = 2283,
      anynonarray_oid = 2776,
      anyenum_oid = 3500
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_PGSQL_OID_HXX
