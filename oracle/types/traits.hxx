// file      : oracle/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <odb/oracle/oracle-types.hxx> // datetime, interval_ym, interval_ds
#include <odb/oracle/traits.hxx>

#include <odb/oracle/details/date.hxx>

#include "test.hxx" // date_time, time_interval

namespace odb
{
  namespace oracle
  {
    template <>
    class value_traits<date_time, id_date>
    {
    public:
      typedef date_time value_type;
      typedef date_time query_type;
      typedef char* image_type;

      static void
      set_value (date_time& v, const char* i, bool is_null)
      {
        if (!is_null)
        {
          short y (0);
          unsigned char m (0), d (0), h (0), mins (0), s (0);

          details::get_date (i, y, m, d, h, mins, s);

          v.year = y;
          v.month = m;
          v.day = d;
          v.hour = h;
          v.minute = mins;
          v.second = s;

          // Oracle DATE does not support fractional seconds.
          //
          v.nanosecond = 0;
        }
      }

      static void
      set_image (char* i, bool& is_null, const date_time& v)
      {
        is_null = false;
        details::set_date (i,
                           static_cast<unsigned short> (v.year),
                           v.month,
                           v.day,
                           v.hour,
                           v.minute,
                           v.second);
      }
    };

    template <>
    class value_traits<date_time, id_timestamp>
    {
    public:
      typedef date_time value_type;
      typedef date_time query_type;
      typedef datetime image_type;

      static void
      set_value (date_time& v, const datetime& i, bool is_null)
      {
        if (!is_null)
        {
          sb2 y (0);
          ub1 m (0), d (0), h (0), mins (0), s (0);
          ub4 ns (0);

          i.get (y, m, d, h, mins, s, ns);

          v.year = y;
          v.month = m;
          v.day = d;
          v.hour = h;
          v.minute = mins;
          v.second = s;
          v.nanosecond = ns;
        }
      }

      static void
      set_image (datetime& i,
                 bool& is_null,
                 const date_time& v)
      {
        is_null = false;

        i.set (static_cast<sb2> (v.year),
               v.month,
               v.day,
               v.hour,
               v.minute,
               v.second,
               v.nanosecond);
      }
    };

    template <>
    class value_traits<time_interval, id_interval_ds>
    {
    public:
      typedef time_interval value_type;
      typedef time_interval query_type;
      typedef interval_ds image_type;

      static void
      set_value (time_interval& v,
                 const interval_ds& i,
                 bool is_null)
      {
        if (!is_null)
        {
          sb4 d (0), h (0), m (0), s (0), ns (0);
          i.get (d, h, m, s, ns);

          v.year = 0;
          v.month = 0;
          v.day = static_cast<unsigned char> (d);
          v.hour = static_cast<unsigned char> (h);
          v.minute = static_cast<unsigned char> (m);
          v.second = static_cast<unsigned char> (s);
          v.nanosecond = static_cast<unsigned int> (ns);
        }
      }

      static void
      set_image (interval_ds& i,
                 bool& is_null,
                 const time_interval& v)
      {
        is_null = false;

        i.set (v.day,
               v.hour,
               v.minute,
               v.second,
               static_cast<sb4> (v.nanosecond));
      }
    };

    template <>
    class value_traits<time_interval, id_interval_ym>
    {
    public:
      typedef time_interval value_type;
      typedef time_interval query_type;
      typedef interval_ym image_type;

      static void
      set_value (time_interval& v,
                 const interval_ym& i,
                 bool is_null)
      {
        if (!is_null)
        {
          sb4 y (0), m (0);
          i.get (y, m);

          v.year = static_cast<unsigned short> (y);
          v.month = static_cast<unsigned char> (m);
          v.day = 0;
          v.hour = 0;
          v.minute = 0;
          v.second = 0;
          v.nanosecond = 0;
        }
      }

      static void
      set_image (interval_ym& i,
                 bool& is_null,
                 const time_interval& v)
      {
        is_null = false;
        i.set (v.year, v.month);
      }
    };
  }
}

#endif // TRAITS_HXX
