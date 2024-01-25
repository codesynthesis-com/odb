// file      : odb/mysql/prepared-query.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_PREPARED_QUERY_HXX
#define ODB_MYSQL_PREPARED_QUERY_HXX

#include <odb/pre.hxx>

#include <odb/prepared-query.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/query.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    struct LIBODB_MYSQL_EXPORT prepared_query_impl: odb::prepared_query_impl
    {
      virtual
      ~prepared_query_impl ();

      prepared_query_impl (odb::connection& c): odb::prepared_query_impl (c) {}

      mysql::query_base query;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_PREPARED_QUERY_HXX
