// file      : odb/mssql/connection.ixx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace mssql
  {
    inline database& connection::
    database ()
    {
      return static_cast<connection_factory&> (factory_).database ();
    }

    template <typename T>
    inline prepared_query<T> connection::
    prepare_query (const char* n, const char* q)
    {
      return prepare_query<T> (n, query<T> (q));
    }

    template <typename T>
    inline prepared_query<T> connection::
    prepare_query (const char* n, const std::string& q)
    {
      return prepare_query<T> (n, query<T> (q));
    }

    template <typename T>
    inline prepared_query<T> connection::
    prepare_query (const char* n, const mssql::query_base& q)
    {
      return query_<T, id_mssql>::call (*this, n, q);
    }

    template <typename T>
    inline prepared_query<T> connection::
    prepare_query (const char* n, const odb::query_base& q)
    {
      // Translate to native query.
      //
      return prepare_query<T> (n, mssql::query_base (q));
    }
  }
}
