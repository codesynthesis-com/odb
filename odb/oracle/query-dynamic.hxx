// file      : odb/oracle/query-dynamic.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_QUERY_DYNAMIC_HXX
#define ODB_ORACLE_QUERY_DYNAMIC_HXX

#include <odb/pre.hxx>

#include <odb/query.hxx>
#include <odb/query-dynamic.hxx>

#include <odb/oracle/query.hxx>

namespace odb
{
  namespace oracle
  {
    typedef details::shared_ptr<query_param> (*query_param_factory) (
      const void* val, const query_column_base&, bool by_ref);

    template <typename T, database_type_id ID>
    details::shared_ptr<query_param>
    query_param_factory_impl (const void*, const query_column_base&, bool);
  }
}

#include <odb/oracle/query-dynamic.ixx>
#include <odb/oracle/query-dynamic.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_QUERY_DYNAMIC_HXX
