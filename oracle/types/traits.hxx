// file      : oracle/types/traits.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <odb/oracle/traits.hxx>

#include "test.hxx" // date_time

namespace odb
{
  namespace oracle
  {
    template <>
    class value_traits<date_time, id_date>
    {
    public:
      typedef long long value_type;
      typedef long long query_type;
      typedef char* image_type;

      static void
      set_value (date_time& v, const char* i, bool is_null)
      {
        if (!is_null)
        {
          v.year = (i[0] - 100) * 100;
          v.year += (i[1] - 100);
          v.month = i[2];
          v.day = i[3];
          v.hour = i[4] - 1;
          v.minute = i[5] - 1;
          v.second = i[6] - 1;
        }
      }

      static void
      set_image (char* i, bool& is_null, const date_time& v)
      {
        is_null = false;

        i[0] = static_cast<char> (v.year / 100 + 100);
        i[1] = static_cast<char> (v.year % 100 + 100);
        i[2] = static_cast<char> (v.month);
        i[3] = static_cast<char> (v.day);
        i[4] = static_cast<char> (v.hour + 1);
        i[5] = static_cast<char> (v.minute + 1);
        i[6] = static_cast<char> (v.second + 1);
      }
    };
  }
}

#endif // TRAITS_HXX
