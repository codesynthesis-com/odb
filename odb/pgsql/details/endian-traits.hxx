// file      : odb/pgsql/details/endian-traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DETAILS_ENDIAN_TRAITS_HXX
#define ODB_PGSQL_DETAILS_ENDIAN_TRAITS_HXX

#include <cstddef> // std::size_t
#include <algorithm> // std::reverse

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  // @@ Revise this.
  //
  namespace details
  {
  }

  namespace pgsql
  {
    namespace details
    {
      using namespace odb::details;

      template <typename T, std::size_t S = sizeof (T)>
      struct swap_endian;

      template <typename T>
      struct swap_endian<T, 1>
      {
        static T
        swap (T x)
        {
          return x;
        }
      };

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

          char tmp (u.c[0]);
          u.c[0] = u.c[1];
          u.c[1] = tmp;

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

          char tmp (u.c[0]);
          u.c[0] = u.c[3];
          u.c[3] = tmp;

          tmp = u.c[1];
          u.c[1] = u.c[2];
          u.c[2] = tmp;

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

          char tmp (u.c[0]);
          u.c[0] = u.c[7];
          u.c[7] = tmp;

          tmp = u.c[1];
          u.c[1] = u.c[6];
          u.c[6] = tmp;

          tmp = u.c[2];
          u.c[2] = u.c[5];
          u.c[5] = tmp;

          tmp = u.c[3];
          u.c[3] = u.c[4];
          u.c[4] = tmp;

          return u.t;
        }
      };

      class LIBODB_PGSQL_EXPORT endian_traits
      {
      public:
        enum endian
        {
          big,
          little
        };

      public:
        static const endian host_endian;

      public:
        template <typename T>
        static T
        hton (T x)
        {
          return host_endian == big ? x : swap_endian<T>::swap (x);
        }

        template <typename T>
        static T
        ntoh (T x)
        {
          return host_endian == big ? x : swap_endian<T>::swap (x);
        }
      };
    }
  }
}

#endif // ODB_PGSQL_DETAILS_ENDIAN_TRAITS_HXX
