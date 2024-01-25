// file      : odb/mysql/query-dynamic.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_QUERY_DYNAMIC_HXX
#define ODB_MYSQL_QUERY_DYNAMIC_HXX

#include <odb/pre.hxx>

#include <odb/query.hxx>
#include <odb/query-dynamic.hxx>

#include <odb/mysql/query.hxx>

namespace odb
{
  namespace mysql
  {
    typedef details::shared_ptr<query_param> (*query_param_factory) (
      const void* val, bool by_ref);

    template <typename T, database_type_id ID>
    details::shared_ptr<query_param>
    query_param_factory_impl (const void*, bool);
  }
}

#include <odb/mysql/query-dynamic.ixx>
#include <odb/mysql/query-dynamic.txx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_QUERY_DYNAMIC_HXX
