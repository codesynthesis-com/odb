// file      : mapping/traits-sqlite.hxx
// copyright : not copyrighted - public domain

#ifndef TRAITS_SQLITE_HXX
#define TRAITS_SQLITE_HXX

//
// SQLite implementation.
//

#include <cstddef> // std::size_t
#include <cstring> // std::strncmp, std::memcpy
#include <sstream>

#include <odb/sqlite/traits.hxx>

#include "person.hxx" // date

namespace odb
{
  namespace sqlite
  {
    template <>
    class value_traits<bool, id_text>
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

    // In SQLite there is no built-in DATE type. Rather, this type is
    // mapped to TEXT in the YYYY-MM-DD format. The below implementation
    // doesn't do any error checking for brevity.
    //
    template <>
    class value_traits<date, id_text>
    {
    public:
      typedef date value_type;
      typedef date query_type;
      typedef details::buffer image_type;

      static void
      set_value (date& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        using namespace std;

        if (!is_null)
        {
          istringstream is (string (b.data (), n));
          unsigned int y, m, d;

          is >> y;
          is.ignore (1);
          is >> m;
          is.ignore (1);
          is >> d;

          v = date (y, m, d);
        }
        else
          v = date (0, 0, 0);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const date& v)
      {
        using namespace std;

        ostringstream os;
        os.fill ('0');

        os.width (4);
        os << v.year () << '-';

        os.width (2);
        os << v.month () << '-';

        os.width (2);
        os << v.day ();

        const string& s (os.str ());

        is_null = false;
        n = s.size ();

        if (n > b.capacity ())
          b.capacity (n);

        memcpy (b.data (), s.c_str (), n);
      }
    };
  }
}

#endif // TRAITS_SQLITE_HXX
