// file      : mssql/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <limits>  // std::numeric_limits
#include <sstream>
#include <cstring> // std::memcpy
#include <cassert>

#include <odb/mssql/traits.hxx>

#include "test.hxx" // variant, point

namespace odb
{
  namespace mssql
  {
    template <>
    class value_traits<variant, id_long_string>
    {
    public:
      typedef variant value_type;
      typedef variant query_type;
      typedef long_callback image_type;

      static void
      set_value (variant& v,
                 result_callback_type& cb,
                 void*& context)
      {
        cb = &result_callback;
        context = &v;
      }

      static void
      set_image (param_callback_type& cb,
                 const void*& context,
                 bool& is_null,
                 const variant& v)
      {
        is_null = false;
        cb = &param_callback;
        context = &v;
      }

      static void
      param_callback (const void* context,
                      std::size_t* position,
                      const void** buffer,
                      std::size_t* size,
                      chunk_type* chunk,
                      void* tmp_buffer,
                      std::size_t tmp_capacity);

      static void
      result_callback (void* context,
                       std::size_t* position,
                       void** buffer,
                       std::size_t* size,
                       chunk_type chunk,
                       std::size_t size_left,
                       void* tmp_buffer,
                       std::size_t tmp_capacity);
    };

    template <>
    struct type_traits<variant>
    {
      static const database_type_id db_type_id = id_long_string;

      struct conversion
      {
        static const char* to () {return "dbo.string_to_variant((?))";}
      };
    };

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    template <>
    class value_traits<point, id_string>
    {
    public:
      typedef point value_type;
      typedef point query_type;

      typedef char* image_type;

      static void
      set_value (point& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = point ();
        else
        {
          // Point format is "POINT (x y)".
          //
          std::istringstream is (std::string (b + 7, n - 7));

          is >> v.x;
          is >> v.y;
        }
      }

      static void
      set_image (char* b,
                 std::size_t c,
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

        os << "POINT (" << v.x << ' ' << v.y << ')';

        const std::string& s (os.str ());
        n = s.size ();
        assert (n <= c);
        std::memcpy (b, s.c_str (), n);
      }
    };

    template <>
    struct type_traits<point>
    {
      static const database_type_id db_type_id = id_string;

      struct conversion
      {
        static const char* to () {return "GEOMETRY::STGeomFromText((?), 0)";}
      };
    };
#endif // SQL Server > 2005
  }
}

#endif // TRAITS_HXX
