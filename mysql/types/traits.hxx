// file      : mysql/types/traits.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <cstring> // std::memcpy, std::memset

#include <odb/mysql/traits.hxx>

#include "test.hxx" // date_time

namespace odb
{
  namespace mysql
  {
    template <database_type_id ID>
    class value_traits<date_time, MYSQL_TIME, ID>
    {
    public:
      typedef date_time value_type;
      typedef date_time query_type;
      typedef MYSQL_TIME image_type;

      static void
      set_value (date_time& v, const MYSQL_TIME& i, bool is_null)
      {
        if (!is_null)
        {
          v.negative = i.neg;
          v.year = i.year;
          v.month = i.month;
          v.day = i.day;
          v.hour = i.hour;
          v.minute = i.minute;
          v.second = i.second;
        }
        else
          v = date_time ();
      }

      static void
      set_image (MYSQL_TIME& i, bool& is_null, const date_time& v)
      {
        is_null = false;
        i.neg = v.negative;
        i.year = v.year;
        i.month = v.month;
        i.day = v.day;
        i.hour = v.hour;
        i.minute = v.minute;
        i.second = v.second;
        i.second_part = 0;
      }
    };

    template <>
    class value_traits<buffer, details::buffer, id_blob>
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
    class value_traits<bitfield, unsigned char*, id_bit>
    {
    public:
      typedef bitfield value_type;
      typedef bitfield query_type;
      typedef unsigned char* image_type;

      static void
      set_value (bitfield& v,
                 const unsigned char* s,
                 std::size_t,
                 bool is_null)
      {
        if (!is_null)
          std::memcpy (&v, s, 1);
        else
          std::memset (&v, 0, sizeof (bitfield));
      }

      static void
      set_image (unsigned char* s,
                 std::size_t,
                 std::size_t& n,
                 bool& is_null,
                 bitfield v)
      {
        is_null = false;
        n = 1;
        std::memcpy (s, &v, 1);
        s[0] &= 0x0F; // Clear unused bits -- MySQL is sensitive about that.
      }
    };

    template <>
    class value_traits<set, details::buffer, id_set>
    {
    public:
      typedef set value_type;
      typedef set query_type;
      typedef details::buffer image_type;

      static void
      set_value (set& v,
                 const details::buffer& b,
                 std::size_t n,
                 bool is_null)
      {
        v.clear ();

        if (!is_null)
        {
          const char* s (b.data ());
          const char* e (s + n);

          while (s < e)
          {
            const char* p (s);

            while (p < e && *p != ',')
              ++p;

            v.insert (std::string (s, p - s));
            s = p;

            if (p != e)
              ++s;
          }
        }
      }

      static void
      set_image (details::buffer& buf,
                 std::size_t& n,
                 bool& is_null,
                 const set& v)
      {
        is_null = false;
        n = 0;

        for (set::const_iterator b (v.begin ()), i (b); i != v.end (); ++i)
        {
          std::size_t m (i->size () + (i != b ? 1 : 0));

          if (n + m > buf.capacity ())
            buf.capacity (n + m, n);

          if (i != b)
            buf.data ()[n++] = ',';

          std::memcpy (buf.data () + n, i->c_str (), i->size ());
          n += i->size ();
        }
      }
    };

    template <>
    class value_traits<std::auto_ptr<std::string>, details::buffer, id_string>
    {
    public:
      typedef std::auto_ptr<std::string> value_type;
      typedef std::string query_type;
      typedef details::buffer image_type;

      static void
      set_value (std::auto_ptr<std::string>& v,
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
                 const std::auto_ptr<std::string>& v)
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
