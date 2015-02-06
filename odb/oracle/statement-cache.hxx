// file      : odb/oracle/statement-cache.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_STATEMENT_CACHE_HXX
#define ODB_ORACLE_STATEMENT_CACHE_HXX

#include <odb/pre.hxx>

#include <map>
#include <typeinfo>

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/statements-base.hxx>

#include <odb/details/shared-ptr.hxx>
#include <odb/details/type-info.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class LIBODB_ORACLE_EXPORT statement_cache
    {
    public:
      statement_cache (connection& conn)
        : conn_ (conn),
          version_seq_ (conn.database ().schema_version_sequence ()) {}

      template <typename T>
      typename object_traits_impl<T, id_oracle>::statements_type&
      find_object ();

      template <typename T>
      view_statements<T>&
      find_view ();

    private:
      typedef std::map<const std::type_info*,
                       details::shared_ptr<statements_base>,
                       details::type_info_comparator> map;

      connection& conn_;
      unsigned int version_seq_;
      map map_;
    };
  }
}

#include <odb/oracle/statement-cache.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_STATEMENT_CACHE_HXX
