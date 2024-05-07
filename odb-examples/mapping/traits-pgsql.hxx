// file      : mapping/traits-pgsql.hxx
// copyright : not copyrighted - public domain

#ifndef TRAITS_PGSQL_HXX
#define TRAITS_PGSQL_HXX

//
// PostgreSQL implementation.
//

#include <cstddef> // std::size_t
#include <cstring> // std::strncmp, std::memset, std::memcpy
#include <ctime>   // localtime, mktime, time_t, tm

#include <odb/pgsql/traits.hxx>
#include <odb/pgsql/details/endian-traits.hxx>

#include "person.hxx" // date

namespace odb
{
  namespace pgsql
  {
    template <>
    class value_traits<bool, id_string>
    {
    public:
      typedef bool value_type;
      typedef bool query_type;
      typedef details::buffer image_type;

      static void
      set_value (bool& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        v = (!is_null && n == 4 && std::strncmp ("true", b.data (), n) == 0);
      }

      static void
      set_image (details::buffer& b,
                 std::size_t& n,
                 bool& is_null,
                 bool v)
      {
        is_null = false;
        n = v ? 4 : 5;

        if (n > b.capacity ())
          b.capacity (n);

        std::memcpy (b.data (), (v ? "true" : "false"), n);
      }
    };

    // Mapping of date to PostgreSQL DATE. DATE is stored as the number
    // of days since the PostgreSQL epoch 2000-01-01.
    //
    template <>
    class value_traits<date, id_date>
    {
    public:
      typedef date value_type;
      typedef date query_type;
      typedef int image_type;

      // The difference between the PostgreSQL epoch and the Unix epoch
      // in seconds.
      //
      static const time_t epoch_diff = 946684800;

      static const time_t seconds_per_day = 86400;

      static void
      set_value (date& v, const int& i, bool is_null)
      {
        if (is_null)
        {
          v = date (0, 0, 0);
          return;
        }

        time_t v_tt (epoch_diff +
                     static_cast<time_t> (details::endian_traits::ntoh (i)) *
                     seconds_per_day);

        // Assume that the date is specified as UTC. Use localtime so as
        // to avoid any timeshift that could be introduced by the system
        // time locale not being UTC.
        //
        tm v_tm (*localtime (&v_tt));

        v = date (v_tm.tm_year + 1900, v_tm.tm_mon + 1, v_tm.tm_mday);
      }

      static void
      set_image (int& i, bool& is_null, const date& v)
      {
        is_null = false;

        tm v_tm;
        std::memset (&v_tm, 0, sizeof (v_tm));

        v_tm.tm_mday = v.day ();
        v_tm.tm_mon = v.month () - 1;
        v_tm.tm_year = v.year () - 1900;

        time_t v_tt (mktime (&v_tm));

        i = details::endian_traits::hton (
          static_cast<int> ((v_tt - epoch_diff) / seconds_per_day));
      }
    };
  }
}

#endif // TRAITS_PGSQL_HXX
