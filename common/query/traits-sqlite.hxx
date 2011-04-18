// file      : common/query/traits-sqlite.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_SQLITE_HXX
#define TRAITS_SQLITE_HXX

#include <string>
#include <memory>  // std::auto_ptr
#include <cstring> // std::memcpy

#include <odb/sqlite/traits.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    class value_traits<std::auto_ptr<std::string>, id_text>
    {
    public:
      typedef std::auto_ptr<std::string> value_type;
      typedef std::string query_type;
      typedef details::buffer image_type;

      static void
      set_value (std::auto_ptr<std::string>& v,
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
                 const std::auto_ptr<std::string>& v)
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

#endif // TRAITS_SQLITE_HXX
