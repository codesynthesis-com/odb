// file      : odb/sqlite/prepared-query.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/sqlite/prepared-query.hxx>

#include <odb/sqlite/connection.hxx>

namespace odb
{
  namespace sqlite
  {
    prepared_query_impl::
    ~prepared_query_impl ()
    {
    }

    bool prepared_query_impl::
    verify_connection (odb::transaction& t)
    {
      // The transaction can be started using the main database of any of the
      // attached databases. So we verify the main connections match.
      //
      return &static_cast<connection&> (t.connection ()).main_connection () ==
        &static_cast<connection&> (stmt->connection ()).main_connection ();
    }
  }
}
