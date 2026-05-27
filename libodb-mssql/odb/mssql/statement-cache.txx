// file      : odb/mssql/statement-cache.txx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/database.hxx>

namespace odb
{
  namespace mssql
  {
    template <typename T>
    typename object_traits_impl<T, id_mssql>::statements_type&
    statement_cache::
    find_object ()
    {
      typedef
      typename object_traits_impl<T, id_mssql>::statements_type
      statements_type;

      // Clear the cache if the database version has changed. This
      // makes sure we don't re-use statements that correspond to
      // the old schema.
      //
      if (version_seq_ != conn_.database ().schema_version_sequence ())
      {
        map_.clear ();
        version_seq_ = conn_.database ().schema_version_sequence ();
      }

      map::iterator i (map_.find (&typeid (T)));

      if (i == map_.end ())
      {
        i = map_.insert (
          map::value_type (
            &typeid (T),
            std::unique_ptr<statements_type> (
              new statements_type (conn_)))).first;
      }

      return static_cast<statements_type&> (*i->second);
    }

    template <typename T>
    view_statements<T>& statement_cache::
    find_view ()
    {
      // We don't cache any statements for views so no need to clear
      // the cache.

      map::iterator i (map_.find (&typeid (T)));

      if (i == map_.end ())
      {
        i = map_.insert (
          map::value_type (
            &typeid (T),
            std::unique_ptr<view_statements<T>> (
              new view_statements<T> (conn_)))).first;
      }

      return static_cast<view_statements<T>&> (*i->second);
    }
  }
}
