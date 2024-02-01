// file      : odb/boost/date-time/sqlite/gregorian-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_SQLITE_GREGORIAN_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_SQLITE_GREGORIAN_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t
#include <cstring>  // std::memcpy
#include <ctime>    // std::time_t

#include <boost/date_time/gregorian/gregorian_types.hpp>    // date
#include <boost/date_time/posix_time/posix_time_types.hpp>  // time_duration
#include <boost/date_time/gregorian/parsers.hpp>      // from_simple_string
#include <boost/date_time/gregorian/formatters.hpp>   // to_iso_extended_string
#include <boost/date_time/posix_time/conversion.hpp>  // from_time_t

#include <odb/core.hxx>
#include <odb/details/buffer.hxx>
#include <odb/sqlite/traits.hxx>

#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    struct default_value_traits< ::boost::gregorian::date, id_text>
    {
      typedef ::boost::gregorian::date date;
      typedef date value_type;
      typedef date query_type;
      typedef details::buffer image_type;

      static void
      set_value (date& v,
                 const details::buffer& i,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = date (::boost::date_time::not_a_date_time);
        else
          v = ::boost::gregorian::from_simple_string (
            std::string (i.data (), n));
      }

      static void
      set_image (details::buffer& i,
                 std::size_t& n,
                 bool& is_null,
                 const date& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else
        {
          is_null = false;

          const std::string& s (
            ::boost::gregorian::to_iso_extended_string (v));

          n = s.size ();

          if (n > i.capacity ())
            i.capacity (n);

          std::memcpy (i.data (), s.data (), n);
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::gregorian::date, id_integer>
    {
      typedef ::boost::gregorian::date date;
      typedef ::boost::posix_time::time_duration time_duration;
      typedef ::boost::posix_time::ptime ptime;
      typedef date value_type;
      typedef date query_type;
      typedef long long image_type;

      static void
      set_value (date& v, long long i, bool is_null)
      {
        if (is_null)
          v = date (::boost::date_time::not_a_date_time);
        else
          v = ::boost::posix_time::from_time_t (
            static_cast<std::time_t> (i)).date ();
      }

      static void
      set_image (long long& i, bool& is_null, const date& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else
        {
          is_null = false;

          ptime epoch (date (1970, 1, 1), time_duration (0, 0, 0));
          i = static_cast<long long> (
            (ptime (v, time_duration (0, 0, 0)) - epoch).ticks () /
            time_duration::ticks_per_second ());
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::gregorian::date>
    {
      static const database_type_id db_type_id = id_text;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_SQLITE_GREGORIAN_TRAITS_HXX
