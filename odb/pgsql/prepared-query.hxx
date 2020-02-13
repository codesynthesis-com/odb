// file      : odb/pgsql/prepared-query.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_PREPARED_QUERY_HXX
#define ODB_PGSQL_PREPARED_QUERY_HXX

#include <odb/pre.hxx>

#include <odb/prepared-query.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/query.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    struct LIBODB_PGSQL_EXPORT prepared_query_impl: odb::prepared_query_impl
    {
      virtual
      ~prepared_query_impl ();

      prepared_query_impl (odb::connection& c): odb::prepared_query_impl (c) {}

      pgsql::query_base query;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_PREPARED_QUERY_HXX
