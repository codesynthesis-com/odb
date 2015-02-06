// file      : pgsql/custom/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <limits>  // std::numeric_limits
#include <vector>
#include <sstream>
#include <cstring> // std::memcpy

#include <odb/pgsql/traits.hxx>

#include "test.hxx" // point

namespace odb
{
  namespace pgsql
  {
    template <>
    class value_traits<point, id_string>
    {
    public:
      typedef point value_type;
      typedef point query_type;
      typedef details::buffer image_type;

      static void
      set_value (point& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = point ();
        else
        {
          // Point format is "(x,y)".
          //
          char c;
          std::istringstream is (std::string (b.data (), n));

          is >> c;    // '('
          is >> v.x;
          is >> c;    // ','
          is >> v.y;
        }
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const point& v)
      {
        is_null = false;
        std::ostringstream os;

        // The formula for the number of decimla digits required is given in:
        //
        // http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1822.pdf
        //
        os.precision (std::numeric_limits<double>::digits10);
        // os.precision (2 + std::numeric_limits<double>::digits * 301/1000);

        os << '(' << v.x << ',' << v.y << ')';

        const std::string& s (os.str ());
        n = s.size ();

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), s.c_str (), n);
      }
    };

    template <>
    struct type_traits<point>
    {
      static const database_type_id db_type_id = id_string;

      struct conversion
      {
        static const char* to () {return "(?)::POINT";}
      };
    };

    template <>
    class value_traits<std::vector<int>, id_string>
    {
    public:
      typedef std::vector<int> value_type;
      typedef value_type query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        v.clear ();

        if (!is_null)
        {
          // Array format is "{n1,n2,n3...}".
          //
          char c;
          std::istringstream is (std::string (b.data (), n));

          is >> c; // '{'

          for (c = static_cast<char> (is.peek ()); c != '}'; is >> c)
          {
            v.push_back (int ());
            is >> v.back ();
          }
        }
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        std::ostringstream os;

        os << '{';

        for (value_type::const_iterator i (v.begin ()), e (v.end ()); i != e;)
        {
          os << *i;

          if (++i != e)
            os << ',';
        }

        os << '}';

        const std::string& s (os.str ());
        n = s.size ();

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), s.c_str (), n);
      }
    };

    template <>
    struct type_traits<std::vector<int> >
    {
      static const database_type_id db_type_id = id_string;

      struct conversion
      {
        static const char* to () {return "(?)::INTEGER[]";}
      };
    };
  }
}

#endif // TRAITS_HXX
