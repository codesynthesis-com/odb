// file      : sqlite/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <cstring> // std::memcpy, std::memset

#include <odb/sqlite/traits.hxx>

#include "test.hxx" // string_ptr

namespace odb
{
  namespace sqlite
  {
    template <>
    class value_traits<string_ptr, id_text>
    {
    public:
      typedef string_ptr value_type;
      typedef std::string query_type;
      typedef details::buffer image_type;

      static void
      set_value (string_ptr& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        v.reset (is_null ? 0 : new std::string (b.data (), n));
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const string_ptr& v)
      {
        is_null = v.get () == 0;

        if (!is_null)
        {
          n = v->size ();

          if (n > b.capacity ())
            b.capacity (n);

          if (n != 0)
            std::memcpy (b.data (), v->c_str (), n);
        }
      }
    };
  }
}

#endif // TRAITS_HXX
