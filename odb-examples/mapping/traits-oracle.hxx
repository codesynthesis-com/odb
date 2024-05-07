// file      : mapping/traits-oracle.hxx
// copyright : not copyrighted - public domain

#ifndef TRAITS_ORACLE_HXX
#define TRAITS_ORACLE_HXX

//
// Oracle implementation.
//

#include <cstddef> // std::size_t
#include <cstring> // std::strncmp, std::memcpy
#include <cassert>

#include <odb/oracle/traits.hxx>
#include <odb/oracle/details/date.hxx>

#include "person.hxx" // date

namespace odb
{
  namespace oracle
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
    class value_traits<date, id_date>
    {
    public:
      typedef date value_type;
      typedef date query_type;
      typedef char* image_type;

      static void
      set_value (date& v, const char* b, bool is_null)
      {
        if (!is_null)
        {
          short y;
          unsigned char m, d, h, min, s;
          details::get_date (b, y, m, d, h, min, s);

          v = date (y, m, d);
        }
        else
          v = date (0, 0, 0);
      }

      static void
      set_image (char* b, bool& is_null, const date& v)
      {
        is_null = false;

        short y (static_cast<short> (v.year ()));
        unsigned char m (static_cast<unsigned char> (v.month ()));
        unsigned char d (static_cast<unsigned char> (v.day ()));

        details::set_date (b, y, m, d, 0, 0, 0);
      }
    };
  }
}

#endif // TRAITS_ORACLE_HXX
