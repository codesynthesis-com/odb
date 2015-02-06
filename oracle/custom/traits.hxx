// file      : oracle/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <vector>
#include <sstream>
#include <cstring> // std::memcpy
#include <cassert> // std::memcpy

#include <odb/oracle/traits.hxx>

namespace odb
{
  namespace oracle
  {

        template <>
    class value_traits<std::vector<int>, id_string>
    {
    public:
      typedef std::vector<int> value_type;
      typedef value_type query_type;
      typedef details::buffer image_type;

      static void
      set_value (value_type& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        v.clear ();

        if (!is_null)
        {
          // Array format is "n1,n2,n3...".
          //
          std::istringstream is (std::string (b, n));

          for (char c; !is.eof (); is >> c)
          {
            v.push_back (int ());
            is >> v.back ();
          }
        }
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const value_type& v)
      {
        is_null = false;
        std::ostringstream os;

        for (value_type::const_iterator i (v.begin ()), e (v.end ()); i != e;)
        {
          os << *i;

          if (++i != e)
            os << ',';
        }

        const std::string& s (os.str ());
        n = s.size ();
        assert (n <= c);
        std::memcpy (b, s.c_str (), n);
      }
    };
  }
}

#endif // TRAITS_HXX
