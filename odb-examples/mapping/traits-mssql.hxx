// file      : mapping/traits-mssql.hxx
// copyright : not copyrighted - public domain

#ifndef TRAITS_MSSQL_HXX
#define TRAITS_MSSQL_HXX

//
// SQL Server implementation.
//

#include <cstddef> // std::size_t
#include <cstring> // std::strncmp, std::memcpy
#include <cassert>

#include <odb/mssql/traits.hxx>

#include "person.hxx" // date

namespace odb
{
  namespace mssql
  {
    template <>
    class value_traits<bool, id_string>
    {
    public:
      typedef bool value_type;
      typedef bool query_type;
      typedef char* image_type;

      static void
      set_value (bool& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        v = (!is_null && n == 4 && std::strncmp ("true", b, n) == 0);
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 bool v)
      {
        is_null = false;
        n = v ? 4 : 5;

        assert (n <= c);

        std::memcpy (b, (v ? "true" : "false"), n);
      }
    };

    template <>
    class value_traits< ::date, id_date>
    {
    public:
      typedef ::date value_type;
      typedef ::date query_type;
      typedef mssql::date image_type;

      static void
      set_value (value_type& v, const image_type& i, bool is_null)
      {
        if (!is_null)
          v = value_type (static_cast<unsigned int> (i.year), i.month, i.day);
        else
          v = value_type (0, 0, 0);
      }

      static void
      set_image (image_type& i, bool& is_null, const value_type& v)
      {
        is_null = false;
        i.year = static_cast<SQLSMALLINT> (v.year ());
        i.month = static_cast<SQLUSMALLINT> (v.month ());
        i.day = static_cast<SQLUSMALLINT> (v.day ());
      }
    };
  }
}

#endif // TRAITS_MSSQL_HXX
