// file      : odb/oracle/details/date.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_DETAILS_DATE_HXX
#define ODB_ORACLE_DETAILS_DATE_HXX

namespace odb
{
  // @@ Revise this.
  //
  namespace details
  {
  }

  namespace oracle
  {
    namespace details
    {
      inline void
      set_date (char* b,
                short year,
                unsigned char month,
                unsigned char day,
                unsigned char hour,
                unsigned char minute,
                unsigned char second)
      {
        b[0] = static_cast<char> (year / 100 + 100);
        b[1] = static_cast<char> (year % 100 + 100);
        b[2] = static_cast<char> (month);
        b[3] = static_cast<char> (day);
        b[4] = static_cast<char> (hour + 1);
        b[5] = static_cast<char> (minute + 1);
        b[6] = static_cast<char> (second + 1);
      }

      inline void
      get_date (const char* b,
                short& year,
                unsigned char& month,
                unsigned char& day,
                unsigned char& hour,
                unsigned char& minute,
                unsigned char& second)
      {
        const unsigned char* ub (reinterpret_cast<const unsigned char*> (b));

        year = 100 * ub[0] + ub[1] - 10100;
        month = ub[2];
        day = ub[3];
        hour = ub[4] - 1;
        minute = ub[5] - 1;
        second = ub[6] - 1;
      }
    }
  }
}

#endif // ODB_ORACLE_DETAILS_DATE_HXX
