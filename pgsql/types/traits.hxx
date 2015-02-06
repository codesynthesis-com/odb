// file      : pgsql/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <cassert>
#include <cstring> // std::memcpy, std::memset

#include <odb/pgsql/traits.hxx>
#include <odb/pgsql/details/endian-traits.hxx>

#include "test.hxx" // varbit, ubuffer, string_ptr

namespace odb
{
  namespace pgsql
  {
    // The first 4 bytes of the image is a signed int specifying the
    // number of significant bits contained by the BIT. The following
    // bytes contain the bit data.
    //
    template <>
    class value_traits<bitfield, id_bit>
    {
    public:
      typedef bitfield value_type;
      typedef bitfield query_type;
      typedef unsigned char* image_type;

      static void
      set_value (bitfield& v,
                 const unsigned char* i,
                 std::size_t,
                 bool is_null)
      {
        if (!is_null)
        {
          assert (
            details::endian_traits::ntoh (
              *reinterpret_cast<const int*> (i)) == 4);

          const unsigned char* d (i + 4);

          v.a = *d >> 4 & 1;
          v.b = (*d >> 5) & 1;
          v.c = (*d >> 6) & 1;
          v.d = (*d >> 7) & 1;
        }
        else
          v.a = v.b = v.c = v.d = 0;
      }

      static void
      set_image (unsigned char* i,
                 std::size_t,
                 std::size_t& n,
                 bool& is_null,
                 bitfield v)
      {
        is_null = false;
        n = 5;

        *reinterpret_cast<int*> (i) = details::endian_traits::hton (4);
        *(i + 4) = v.a << 4 | (v.b << 5) | (v.c << 6) | (v.d << 7);
      }
    };

    // The first 4 bytes of the image is a signed int specifying the
    // number of significant bits contained by the VARBIT. The following
    // bytes contain the VARBIT data.
    //
    template <>
    class value_traits<varbit, id_varbit>
    {
    public:
      typedef varbit value_type;
      typedef varbit query_type;
      typedef details::ubuffer image_type;

      static void
      set_value (varbit& v,
                 const details::ubuffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
        {
          v.size = static_cast<std::size_t> (
            details::endian_traits::ntoh (
              *reinterpret_cast<const int*> (b.data ())));

          std::size_t byte_len (v.size / 8 + (v.size % 8 > 0 ? 1 : 0));
          assert (n >= byte_len + 4);

          v.ubuffer_.assign (b.data () + 4, byte_len);
        }

        else
        {
          v.size = 0;
          v.ubuffer_.assign (0, 0);
        }
      }

      static void
      set_image (details::ubuffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const varbit& v)
      {
        is_null = false;
        n = 4 + v.size / 8 + (v.size % 8 > 0 ? 1 : 0);

        if (n > b.capacity ())
          b.capacity (n);

        // PostgreSQL requires all trailing bits of a VARBIT image
        // to be zero.
        //
        std::memset (b.data (), 0, b.capacity ());

        *reinterpret_cast<int*> (b.data ()) =
          details::endian_traits::hton (static_cast<int> (v.size));

        if (v.size != 0)
          std::memcpy (b.data () + 4, v.ubuffer_.data (), n - 4);
      }
    };

    template <>
    class value_traits<string_ptr, id_string>
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
