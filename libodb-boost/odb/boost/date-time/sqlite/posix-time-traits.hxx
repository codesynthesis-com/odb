// file      : odb/boost/date-time/sqlite/posix-time-traits.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_SQLITE_POSIX_TIME_TRAITS_HXX
#define ODB_BOOST_DATE_TIME_SQLITE_POSIX_TIME_TRAITS_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t
#include <cstring>  // std::memcpy
#include <ctime>    // std::time_t

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>     // time_from_string
#include <boost/date_time/posix_time/time_formatters.hpp>  // to_simple_string
#include <boost/date_time/posix_time/conversion.hpp>       // from_time_t

#include <odb/core.hxx>
#include <odb/details/buffer.hxx>
#include <odb/sqlite/traits.hxx>

#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace sqlite
  {
    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_text>
    {
      typedef ::boost::posix_time::ptime ptime;
      typedef ptime value_type;
      typedef ptime query_type;
      typedef details::buffer image_type;

      static void
      set_value (ptime& v,
                 const details::buffer& i,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
          v = ::boost::posix_time::time_from_string (
            std::string (i.data (), n));
      }

      static void
      set_image (details::buffer& i,
                 std::size_t& n,
                 bool& is_null,
                 const ptime& v)
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

          std::string s (::boost::posix_time::to_iso_extended_string (v));

          // Replace ',' in iso string with '.'. SQLite requires the
          // latter for date/time functions.
          //
          std::size_t p = s.rfind (',');
          if (p != std::string::npos)
            s[p] = '.';

          // Remove 'T' separator as Boost is unable to parse correctly.
          //
          p = s.find ('T');
          if (p != std::string::npos)
            s[p] = ' ';

          n = s.size ();
          if (n > i.capacity ())
            i.capacity (n);

          std::memcpy (i.data (), s.data (), n);
        }
      }
    };

    // Implementation of the mapping between boost::posix_time::ptime and
    // SQLite INTEGER. The integer value represents UNIX time.
    //
    template <>
    struct default_value_traits< ::boost::posix_time::ptime, id_integer>
    {
      typedef ::boost::gregorian::date date;
      typedef ::boost::posix_time::ptime ptime;
      typedef ::boost::posix_time::time_duration time_duration;
      typedef ptime value_type;
      typedef ptime query_type;
      typedef long long image_type;

      static void
      set_value (ptime& v, long long i, bool is_null)
      {
        if (is_null)
          v = ptime (::boost::date_time::not_a_date_time);
        else
          v = ::boost::posix_time::from_time_t (static_cast<std::time_t> (i));
      }

      static void
      set_image (long long& i, bool& is_null, const ptime& v)
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

          ptime epoch (date (1970, 1, 1), time_duration (0, 0, 0));
          i = static_cast<long long> (
            (v - epoch).ticks () / time_duration::ticks_per_second ());
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::posix_time::time_duration, id_text>
    {
      typedef ::boost::posix_time::time_duration time_duration;
      typedef time_duration value_type;
      typedef time_duration query_type;
      typedef details::buffer image_type;

      static void
      set_value (time_duration& v,
                 const details::buffer& i,
                 std::size_t n,
                 bool is_null)
      {
        if (is_null)
          v = time_duration (::boost::date_time::not_a_date_time);
        else
        {
          v = ::boost::posix_time::duration_from_string (
            std::string (i.data (), n));
        }
      }

      static void
      set_image (details::buffer& i,
                 std::size_t& n,
                 bool& is_null,
                 const time_duration& v)
      {
        if (v.is_special ())
        {
          if (v.is_not_a_date_time ())
            is_null = true;
          else
            throw odb::boost::date_time::special_value ();
        }
        else if (v.total_seconds () < 0)
          throw odb::boost::date_time::value_out_of_range ();
        else
        {
          is_null = false;

          const std::string& s (::boost::posix_time::to_simple_string (v));

          n = s.size ();
          if (n > i.capacity ())
            i.capacity (n);

          std::memcpy (i.data (), s.data (), n);
        }
      }
    };

    template <>
    struct default_value_traits< ::boost::posix_time::time_duration, id_integer>
    {
      typedef ::boost::posix_time::time_duration time_duration;
      typedef time_duration value_type;
      typedef time_duration query_type;
      typedef details::buffer image_type;

      static void
      set_value (time_duration& v, long long i, bool is_null)
      {
        if (is_null)
          v = time_duration (::boost::date_time::not_a_date_time);
        else
          v = time_duration (0, 0, static_cast<time_duration::sec_type> (i));
      }

      static void
      set_image (long long& i, bool& is_null, const time_duration& v)
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
          i = static_cast<long long> (v.total_seconds ());
        }
      }
    };

    template <>
    struct default_type_traits< ::boost::posix_time::ptime>
    {
      static const database_type_id db_type_id = id_text;
    };

    template <>
    struct default_type_traits< ::boost::posix_time::time_duration>
    {
      static const database_type_id db_type_id = id_text;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_BOOST_DATE_TIME_SQLITE_POSIX_TIME_TRAITS_HXX
