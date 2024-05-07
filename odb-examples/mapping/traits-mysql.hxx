// file      : mapping/traits-mysql.hxx
// copyright : not copyrighted - public domain

#ifndef TRAITS_MYSQL_HXX
#define TRAITS_MYSQL_HXX

//
// MySQL implementation.
//

#include <cstddef> // std::size_t
#include <cstring> // std::strncmp, std::memcpy

#include <odb/mysql/traits.hxx>

#include "person.hxx" // date

namespace odb
{
  namespace mysql
  {
    template <>
    class value_traits<bool, id_string>
    {
    public:
      typedef bool value_type;
      typedef bool query_type;
      typedef details::buffer image_type;

      static void
      set_value (bool& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        v = (!is_null && n == 4 && std::strncmp ("true", b.data (), n) == 0);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 bool v)
      {
        is_null = false;
        n = v ? 4 : 5;

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), (v ? "true" : "false"), n);
      }
    };

    template <>
    class value_traits<date, id_date>
    {
    public:
      typedef date value_type;
      typedef date query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (date& v, const MYSQL_TIME& i, bool is_null)
      {
        if (!is_null)
          v = date (i.year, i.month, i.day);
        else
          v = date (0, 0, 0);
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const date& v)
      {
        is_null = false;
        i.neg = false;
        i.year = v.year ();
        i.month = v.month ();
        i.day = v.day ();
      }
    };
  }
}

#endif // TRAITS_MYSQL_HXX
