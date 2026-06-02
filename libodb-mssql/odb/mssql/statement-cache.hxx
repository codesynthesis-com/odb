// file      : odb/mssql/statement-cache.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_STATEMENT_CACHE_HXX
#define ODB_MSSQL_STATEMENT_CACHE_HXX

#include <odb/pre.hxx>

#include <map>
#include <memory>  // std::unique_ptr
#include <typeinfo>

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/statements-base.hxx>

#include <odb/details/type-info.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class LIBODB_MSSQL_EXPORT statement_cache
    {
    public:
      statement_cache (connection& conn)
        : conn_ (conn),
          version_seq_ (conn_.database ().schema_version_sequence ()) {}

      template <typename T>
      typename object_traits_impl<T, id_mssql>::statements_type&
      find_object ();

      template <typename T>
      view_statements<T>&
      find_view ();

      statement_cache (const statement_cache&) = delete;
      statement_cache& operator= (const statement_cache&) = delete;

    private:
      typedef std::map<const std::type_info*,
                       std::unique_ptr<statements_base>,
                       details::type_info_comparator> map;

      connection& conn_;
      unsigned int version_seq_;
      map map_;
    };
  }
}

#include <odb/mssql/statement-cache.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_STATEMENT_CACHE_HXX
