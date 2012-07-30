// file      : odb/boost/date-time/mysql/posix-time-traits.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_MYSQL_POSIX_TIME_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_MYSQL_POSIX_TIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <cstdlib>  // std::abs

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <odb/core.hxx>
#include <odb/mysql/traits.hxx>
#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace mysql
  {
    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_datetime>
    {
      typedef ::boost::posix_time::ptime ptime;
      typedef ::boost::posix_time::time_duration time_duration;
      typedef ::boost::gregorian::date date;
      typedef ptime value_type;
      typedef ptime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (ptime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
          v = ptime (date (i.year, i.month, i.day),
                     time_duration (i.hour, i.minute, i.second));
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const ptime& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else
        {
          is_null = false;

          const date& d (v.date ());
          i.year = d.year ();
          i.month = d.month ();
          i.day = d.day ();

          const time_duration& t (v.time_of_day ());
          i.hour = t.hours ();
          i.minute = t.minutes ();
          i.second = t.seconds ();
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_timestamp>
    {
      typedef ::boost::posix_time::ptime ptime;
      typedef ::boost::posix_time::time_duration time_duration;
      typedef ::boost::gregorian::date date;
      typedef ptime value_type;
      typedef ptime query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (ptime& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
          v = ptime (date (i.year, i.month, i.day),
                     time_duration (i.hour, i.minute, i.second));
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const ptime& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else if (v < ptime (date (1970, ::boost::date_time::Jan, 1),
                            time_duration (0, 0, 1)) ||
                 v > ptime (date (2038, ::boost::date_time::Jan, 19),
                            time_duration (3, 14, 7)))
          throw odb::boost::date_time::value_out_of_range ();
        else
        {
          is_null = false;

          const date& d (v.date ());
          i.year = d.year ();
          i.month = d.month ();
          i.day = d.day ();

          const time_duration& t (v.time_of_day ());
          i.hour = t.hours ();
          i.minute = t.minutes ();
          i.second = t.seconds ();
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::posix_time::time_duration, id_time>
    {
      typedef ::boost::posix_time::time_duration time_duration;
      typedef time_duration value_type;
      typedef time_duration query_type;
      typedef MYSQL_TIME image_type;

      static const unsigned short max_hours = 838;

      static void
      set_value (time_duration& v, const MYSQL_TIME& i, bool is_null)
      {
        if (is_null)
          v = time_duration (::boost::date_time::not_a_date_time);
        else
        {
          v = time_duration (i.hour, i.minute, i.second);

          if (i.neg)
            v = v.invert_sign ();
        }
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const time_duration& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else if (std::abs (v.hours ()) > max_hours)
          throw odb::boost::date_time::value_out_of_range ();
        else
        {
          is_null = false;
          i.day = 0;
          i.hour = std::abs (v.hours ());
          i.minute = std::abs (v.minutes ());
          i.second = std::abs (v.seconds ());
          i.neg = v.is_negative ();
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::posix_time::ptime>
    {
      static const database_type_id db_type_id = id_datetime;
    };

    template <>
    struct default_type_traits< ::boost::posix_time::time_duration>
    {
      static const database_type_id db_type_id = id_time;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_MYSQL_POSIX_TIME_TRAITS_HXX
