// file      : odb/mssql/prepared-query.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_PREPARED_QUERY_HXX
#define ODB_MSSQL_PREPARED_QUERY_HXX

#include <odb/pre.hxx>

#include <odb/prepared-query.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/query.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    struct LIBODB_MSSQL_EXPORT prepared_query_impl: odb::prepared_query_impl
    {
      virtual
      ~prepared_query_impl ();

      prepared_query_impl (odb::connection& c): odb::prepared_query_impl (c) {}

      mssql::query_base query;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_PREPARED_QUERY_HXX
