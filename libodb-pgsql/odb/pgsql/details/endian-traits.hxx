// file      : odb/pgsql/details/endian-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_DETAILS_ENDIAN_TRAITS_HXX
#define ODB_PGSQL_DETAILS_ENDIAN_TRAITS_HXX

#include <cstddef> // std::size_t
#include <algorithm> // std::reverse

#include <odb/pgsql/details/export.hxx>

// Note: the following byte order macro definitions are copied from the
//       BYTE_ORDER.h header file of the libbuild2-autoconf package.

/* Include the endianness header based on platform.
 *
 * Each of these headers should define BYTE_ORDER, LITTLE_ENDIAN, BIG_ENDIAN,
 * AND PDP_ENDIAN but this can be affected by macros like _ANSI_SOURCE,
 * _POSIX_C_SOURCE, _XOPEN_SOURCE and _NETBSD_SOURCE, depending on the
 * platform (in which case most of them define underscored versions only).
 */
#if defined(__GLIBC__) || defined(__OpenBSD__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__APPLE__)
#  include <machine/endian.h>
#elif !defined(_WIN32)
#  include <sys/param.h>
#endif

/* Try various system- and compiler-specific byte order macro names if the
 * endianness headers did not define BYTE_ORDER.
 */
#if !defined(BYTE_ORDER)
#  if defined(__linux__)
#    if defined(__BYTE_ORDER)
#      define BYTE_ORDER    __BYTE_ORDER
#      define BIG_ENDIAN    __BIG_ENDIAN
#      define LITTLE_ENDIAN __LITTLE_ENDIAN
#    endif
#  elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#    if defined(_BYTE_ORDER)
#      define BYTE_ORDER    _BYTE_ORDER
#      define BIG_ENDIAN    _BIG_ENDIAN
#      define LITTLE_ENDIAN _LITTLE_ENDIAN
#    endif
#  elif defined(__APPLE__)
#    if defined(__DARWIN_BYTE_ORDER)
#      define BYTE_ORDER    __DARWIN_BYTE_ORDER
#      define BIG_ENDIAN    __DARWIN_BIG_ENDIAN
#      define LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
#    endif
#  elif defined(_WIN32)
#    define BIG_ENDIAN    4321
#    define LITTLE_ENDIAN 1234
#    define BYTE_ORDER    LITTLE_ENDIAN
#  elif defined(__BYTE_ORDER__) &&       \
        defined(__ORDER_BIG_ENDIAN__) && \
        defined(__ORDER_LITTLE_ENDIAN__)
     /* GCC, Clang (and others, potentially).
      */
#    define BYTE_ORDER    __BYTE_ORDER__
#    define BIG_ENDIAN    __ORDER_BIG_ENDIAN__
#    define LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#  endif
#endif

#ifndef BYTE_ORDER
#  error no byte order macros defined
#endif

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
        template <typename T>
        static T
        hton (T x)
        {
#if BYTE_ORDER == BIG_ENDIAN
          return x;
#else
          return swap_endian<T>::swap (x);
#endif
        }

        template <typename T>
        static T
        ntoh (T x)
        {
#if BYTE_ORDER == BIG_ENDIAN
          return x;
#else
          return swap_endian<T>::swap (x);
#endif
        }
      };
    }
  }
}

#endif // ODB_PGSQL_DETAILS_ENDIAN_TRAITS_HXX
