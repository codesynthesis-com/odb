// file      : mysql/types/traits.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <cstring> // std::memcpy, std::memset

#include <odb/mysql/traits.hxx>

#include "test.hxx" // date_time, string_ptr

namespace odb
{
  namespace mysql
  {
    template <database_type_id ID>
    class value_traits<date_time, ID>
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
          v.microseconds = static_cast<unsigned int> (i.second_part);
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
        i.second_part = v.microseconds;
      }
    };

    template <>
    class value_traits<bitfield, id_bit>
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
        {
          v.a = *s & 1;
          v.b = (*s >> 1) & 1;
          v.c = (*s >> 2) & 1;
          v.d = (*s >> 3) & 1;
        }
        else
          v.a = v.b = v.c = v.d = 0;
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
        *s = v.a | (v.b << 1) | (v.c << 2) | (v.d << 3);
      }
    };

    template <>
    class value_traits<set, id_set>
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
    class value_traits<string_ptr, id_string>
    {
    public:
      typedef string_ptr value_type;
      typedef std::string query_type;
      typedef details::buffer image_type;

      static void
      set_value (string_ptr& v,
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
                 const string_ptr& v)
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
