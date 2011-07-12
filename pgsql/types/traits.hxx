// file      : pgsql/types/traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <cstring> // std::memcpy, std::memset

#include <odb/pgsql/traits.hxx>
#include <odb/pgsql/details/endian-traits.hxx>

#include "test.hxx" // date_time, buffer, string_ptr

namespace odb
{
  namespace pgsql
  {
    template <>
    class value_traits<buffer, id_bytea>
    {
    public:
      typedef buffer value_type;
      typedef buffer query_type;
      typedef details::buffer image_type;

      static void
      set_value (buffer& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          v.assign (b.data (), n);
        else
          v.assign (0, 0);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const buffer& v)
      {
        is_null = false;
        n = v.size ();

        if (n > b.capacity ())
          b.capacity (n);

        if (n != 0)
          std::memcpy (b.data (), v.data (), n);
      }
    };

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
          unsigned char const* d = i + 4;

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

    template <>
    class value_traits<buffer, id_varbit>
    {
    public:
      typedef buffer value_type;
      typedef buffer query_type;
      typedef details::ubuffer image_type;

      static void
      set_value (buffer& v,
                 const details::ubuffer& b,
                 std::size_t n,
                 bool is_null)
      {
        if (!is_null)
          v.assign (b.data () + 4, n - 4);
        else
          v.assign (0, 0);
      }

      static void
      set_image (details::ubuffer& b,
                 std::size_t& n,
                 bool& is_null,
                 const buffer& v)
      {
        is_null = false;
        n = v.size () + 4;

        if (n > b.capacity ())
          b.capacity (n);

        int bit_len = static_cast<int> (v.size () * 8);

        *reinterpret_cast<int*> (b.data ()) =
          details::endian_traits::hton (bit_len);

        if (bit_len != 0)
          std::memcpy (b.data () + 4, v.data (), v.size ());
      }
    };

    template <>
    class value_traits<unsigned char[16], id_uuid>
    {
    public:
      typedef unsigned char* value_type;
      typedef details::ubuffer query_type;
      typedef unsigned char* image_type;

      static void
      set_value (unsigned char v[16],
                 unsigned char const* i,
                 bool is_null)
      {
        if (!is_null)
          std::memcpy (v, i, 16);
        else
          std::memset (v, 0, 16);
      }

      static void
      set_image (unsigned char* i, bool& is_null, unsigned char const v[16])
      {
        is_null = false;
        std::memcpy (i, v, 16);
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
