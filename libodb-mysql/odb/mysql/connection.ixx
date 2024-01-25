// file      : odb/mysql/connection.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace mysql
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
    prepare_query (const char* n, const mysql::query_base& q)
    {
      return query_<T, id_mysql>::call (*this, n, q);
    }

    template <typename T>
    inline prepared_query<T> connection::
    prepare_query (const char* n, const odb::query_base& q)
    {
      // Translate to native query.
      //
      return prepare_query<T> (n, mysql::query_base (q));
    }
  }
}
