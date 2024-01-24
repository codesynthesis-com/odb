// file      : odb/sqlite/statement-cache.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_STATEMENT_CACHE_HXX
#define ODB_SQLITE_STATEMENT_CACHE_HXX

#include <odb/pre.hxx>

#include <map>
#include <typeinfo>

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/details/shared-ptr.hxx>
#include <odb/details/type-info.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/statement.hxx>
#include <odb/sqlite/statements-base.hxx>

#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    class LIBODB_SQLITE_EXPORT statement_cache
    {
    public:
      statement_cache (connection& conn)
          : conn_ (conn),
            version_seq_ (conn_.database ().schema_version_sequence ()) {}

      template <typename T>
      typename object_traits_impl<T, id_sqlite>::statements_type&
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

#include <odb/sqlite/statement-cache.txx>

#include <odb/post.hxx>

#endif // ODB_SQLITE_STATEMENT_CACHE_HXX
