// file      : mssql/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <odb/mssql/mssql-fwd.hxx> // date, time, datetime, datetimeoffset
#include <odb/mssql/traits.hxx>

#include "test.hxx" // date_time

namespace odb
{
  namespace mssql
  {
    template <>
    class value_traits<date_time, id_date>
    {
    public:
      typedef date_time value_type;
      typedef date_time query_type;
      typedef date image_type;

      static void
      set_value (date_time& v, const date& i, bool is_null)
      {
        if (!is_null)
        {
          v.year = i.year;
          v.month = i.month;
          v.day = i.day;
          v.hour = 0;
          v.minute = 0;
          v.second = 0;
          v.fraction = 0;
          v.timezone_hour = 0;
          v.timezone_minute = 0;
        }
      }

      static void
      set_image (date& i, bool& is_null, const date_time& v)
      {
        is_null = false;
        i.year = v.year;
        i.month = v.month;
        i.day = v.day;
      }
    };

    template <>
    class value_traits<date_time, id_time>
    {
    public:
      typedef date_time value_type;
      typedef date_time query_type;
      typedef time image_type;

      static void
      set_value (date_time& v, const time& i, bool is_null)
      {
        if (!is_null)
        {
          v.year = 0;
          v.month = 0;
          v.day = 0;
          v.hour = i.hour;
          v.minute = i.minute;
          v.second = i.second;
          v.fraction = i.fraction;
          v.timezone_hour = 0;
          v.timezone_minute = 0;
        }
      }

      static void
      set_image (time& i, unsigned short s, bool& is_null, const date_time& v)
      {
        const unsigned int divider[8] =
        {
          1000000000,
          100000000,
          10000000,
          1000000,
          100000,
          10000,
          1000,
          100
        };

        is_null = false;
        i.hour = v.hour;
        i.minute = v.minute;
        i.second = v.second;
        i.fraction = v.fraction - v.fraction % divider[s];
      }
    };

    template <>
    class value_traits<date_time, id_datetime>
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
          v.year = i.year;
          v.month = i.month;
          v.day = i.day;
          v.hour = i.hour;
          v.minute = i.minute;
          v.second = i.second;
          v.fraction = i.fraction;
          v.timezone_hour = 0;
          v.timezone_minute = 0;
        }
      }

      static void
      set_image (datetime& i,
                 unsigned short s,
                 bool& is_null,
                 const date_time& v)
      {
        const unsigned int divider[8] =
        {
          1000000000,
          100000000,
          10000000,
          1000000,
          100000,
          10000,
          1000,
          100
        };

        is_null = false;
        i.year = v.year;
        i.month = v.month;
        i.day = v.day;
        i.hour = v.hour;
        i.minute = v.minute;

        // Scale value 8 indicates we are dealing with SMALLDATETIME
        // which has the minutes precision.
        //
        if (s != 8)
        {
          i.second = v.second;
          i.fraction = v.fraction - v.fraction % divider[s];
        }
        else
        {
          i.second = 0;
          i.fraction = 0;
        }
      }
    };

    template <>
    class value_traits<date_time, id_datetimeoffset>
    {
    public:
      typedef date_time value_type;
      typedef date_time query_type;
      typedef datetimeoffset image_type;

      static void
      set_value (date_time& v, const datetimeoffset& i, bool is_null)
      {
        if (!is_null)
        {
          v.year = i.year;
          v.month = i.month;
          v.day = i.day;
          v.hour = i.hour;
          v.minute = i.minute;
          v.second = i.second;
          v.fraction = i.fraction;
          v.timezone_hour = i.timezone_hour;
          v.timezone_minute = i.timezone_minute;
        }
      }

      static void
      set_image (datetimeoffset& i,
                 unsigned short s,
                 bool& is_null,
                 const date_time& v)
      {
        const unsigned int divider[8] =
        {
          1000000000,
          100000000,
          10000000,
          1000000,
          100000,
          10000,
          1000,
          100
        };

        is_null = false;
        i.year = v.year;
        i.month = v.month;
        i.day = v.day;
        i.hour = v.hour;
        i.minute = v.minute;
        i.second = v.second;
        i.fraction = v.fraction - v.fraction % divider[s];
        i.timezone_hour = v.timezone_hour;
        i.timezone_minute = v.timezone_minute;
      }
    };
  }
}

#endif // TRAITS_HXX
