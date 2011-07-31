// file      : odb/pgsql/traits.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstring> // std::memcpy, std::strlen

#include <odb/pgsql/traits.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    using details::buffer;

    //
    // string_value_traits
    //

    void string_value_traits::
    set_image (buffer& b,
               size_t& n,
               bool& is_null,
               const string& v)
    {
      is_null = false;
      n = v.size ();

      if (n > b.capacity ())
        b.capacity (n);

      if (n != 0)
        memcpy (b.data (), v.c_str (), n);
    }

    //
    // c_string_value_traits
    //

    void c_string_value_traits::
    set_image (buffer& b,
               size_t& n,
               bool& is_null,
               const char* v)
    {
      is_null = false;
      n = strlen (v);

      if (n > b.capacity ())
        b.capacity (n);

      if (n != 0)
        memcpy (b.data (), v, n);
    }

    //
    // default_value_traits<vector<char>, id_bytea>
    //

    void default_value_traits<vector<char>, id_bytea>::
    set_image (details::buffer& b,
               size_t& n,
               bool& is_null,
               const value_type& v)
    {
      is_null = false;
      n = v.size ();

      if (n > b.capacity ())
        b.capacity (n);

      // std::vector::data() may not be available in older compilers.
      //
      if (n != 0)
        memcpy (b.data (), &v.front (), n);
    }
  }
}
