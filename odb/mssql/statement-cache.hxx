// file      : odb/mssql/statement-cache.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_STATEMENT_CACHE_HXX
#define ODB_MSSQL_STATEMENT_CACHE_HXX

#include <odb/pre.hxx>

#include <map>
#include <typeinfo>

#include <odb/forward.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/statements-base.hxx>
#include <odb/mssql/object-statements.hxx>
#include <odb/mssql/view-statements.hxx>

#include <odb/details/shared-ptr.hxx>
#include <odb/details/type-info.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class connection;

    class LIBODB_MSSQL_EXPORT statement_cache
    {
    public:
      statement_cache (connection& conn)
          : conn_ (conn)
      {
      }

      template <typename T>
      typename object_statements_selector<T>::type&
      find_object ()
      {
        typedef typename object_statements_selector<T>::type object_statements;

        map::iterator i (map_.find (&typeid (T)));

        if (i != map_.end ())
          return static_cast<object_statements&> (*i->second);

        details::shared_ptr<object_statements> p (
          new (details::shared) object_statements (conn_));

        map_.insert (map::value_type (&typeid (T), p));
        return *p;
      }

      template <typename T>
      view_statements<T>&
      find_view ()
      {
        map::iterator i (map_.find (&typeid (T)));

        if (i != map_.end ())
          return static_cast<view_statements<T>&> (*i->second);

        details::shared_ptr<view_statements<T> > p (
          new (details::shared) view_statements<T> (conn_));

        map_.insert (map::value_type (&typeid (T), p));
        return *p;
      }

    private:
      typedef std::map<const std::type_info*,
                       details::shared_ptr<statements_base>,
                       details::type_info_comparator> map;

      connection& conn_;
      map map_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_STATEMENT_CACHE_HXX
