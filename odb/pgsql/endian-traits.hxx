// file      : odb/pgsql/endian-traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_ENDIAN_TRAITS_HXX
#define ODB_PGSQL_ENDIAN_TRAITS_HXX

#include <algorithm> // std::reverse

namespace odb
{
  namespace pgsql
  {
    template <typename T, int S = sizeof (T)>
    struct swap_endian;

    template <typename T>
    struct swap_endian<T, 2>
    {
      static T
      swap (T x)
      {
        union u2
        {
          T t;
          char c[2];
        };

        u2 u;
        u.t = x;
        std::reverse (u.c, u.c + 2);

        return u.t;
      }
    };

    template <typename T>
    struct swap_endian<T, 4>
    {
      static T
      swap (T x)
      {
        union u4
        {
          T t;
          char c[4];
        };

        u4 u;
        u.t = x;
        std::reverse (u.c, u.c + 4);

        return u.t;
      }
    };

    template <typename T>
    struct swap_endian<T, 8>
    {
      static T
      swap (T x)
      {
        union u8
        {
          T t;
          char c[8];
        };

        u8 u;
        u.t = x;
        std::reverse (u.c, u.c + 8);

        return u.t;
      }
    };

    class endian_traits
    {
    public:
      enum endian
      {
        big_endian,
        little_endian
      };

    public:
      static const endian host_endian;

    public:

      template <typename T>
      static T
      hton (T x)
      {
        return host_endian == big_endian ? x : swap_endian<T>::swap (x);
      }

      template <typename T>
      static T
      ntoh (T x)
      {
        return host_endian == big_endian ? x : swap_endian<T>::swap (x);
      }
    };
  }
}

#endif // ODB_PGSQL_ENDIAN_TRAITS_HXX
