// file      : odb/mssql/query-dynamic.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_QUERY_DYNAMIC_HXX
#define ODB_MSSQL_QUERY_DYNAMIC_HXX

#include <odb/pre.hxx>

#include <odb/query.hxx>
#include <odb/query-dynamic.hxx>

#include <odb/mssql/query.hxx>

namespace odb
{
  namespace mssql
  {
    typedef details::shared_ptr<query_param> (*query_param_factory) (
      const void* val, const query_column_base&, bool by_ref);

    template <typename T, database_type_id ID>
    details::shared_ptr<query_param>
    query_param_factory_impl (const void*, const query_column_base&, bool);
  }
}

#include <odb/mssql/query-dynamic.ixx>
#include <odb/mssql/query-dynamic.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_QUERY_DYNAMIC_HXX
