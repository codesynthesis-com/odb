// file      : odb/oracle/connection.ixx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
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
    prepare_query (const char* n, const query<T>& q)
    {
      return query_<T, id_oracle>::call (*this, n, q);
    }
  }
}
