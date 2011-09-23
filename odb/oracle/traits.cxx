// file      : odb/oracle/traits.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cmath>
#include <deque>

#include <odb/oracle/traits.hxx>

namespace odb
{
  namespace oracle
  {
    long long
    out (unsigned char const* n)
    {
      // The first byte specifies the length of the number representation,
      // including the exponent byte and significant digits but not the length
      // itself.
      //
      int len (n[0]);

      if (len == 1)
        return 0;

      long long v (0);
      if (n[1] & 0x80)
      {
        // The exponent of a positive number is calculated as n[1] - 193.
        // The base-100 exponent of the least significant digit can be
        // calculated as exp - sig - 1, where sig is the number of significant
        // digits. sig may be calculated as len - 2.
        //
        int exp (n[1] - 191 - len);

        for (unsigned char const* m (n + len), *e (n + 1); m > e; --m, ++exp)
          v += (*m - 1) * pow (100, exp);
      }
      else
      {
        // The exponent of a negative number is calculated as ~n[1] - 193.
        // The base-100 exponent of the least significant digit can be
        // calculated as exp - sig - 1, where sig is the number of significant
        // digits. sig may be calculated as len - 2.
        //
        int exp (~n[1] - 190 - len);

        for (unsigned char const* m (n + len - 1), *e (n + 1); m > e;
             --m, ++exp)
          v -= (101 - *m) * pow (100, exp);
      }

      return v;
    }

    void
    in (unsigned char b[22], long long n)
    {
      if (n == 0)
      {
        b[0] = 1;
        b[1] = 128;

        return;
      }

      // @@ Is there some sort of reserve() implementation for deque?
      //
      deque<unsigned char> s;

      bool sig (false);
      size_t c (0);
      unsigned char exp (0);

      if (n < 0)
      {
        // Termination marker for negative numbers.
        //
        s.push_front (102);

        while (n != 0)
        {
          int v (static_cast<int> (n % 100));
          sig = sig ? true : v != 0;

          if (sig)
            s.push_front (static_cast<unsigned char> (101 + v));

          n /= 100;
          ++c;
        }

        // The exponent is one less than the number of base 100 digits. It is
        // inverted for negative values.
        //
        exp = static_cast<unsigned char> (~(c + 192));
      }
      else
      {
        while (n != 0)
        {
          int v (static_cast<int> (n % 100));
          sig = sig ? true : v != 0;

          if (sig)
            s.push_front (static_cast<unsigned char> (v + 1));

          n /= 100;
          ++c;
        }

        // Exponent is one less than the number of base 100 digits.
        //
        exp = static_cast<unsigned char> (c + 192);
      }

      // Set the length.
      //
      b[0] = static_cast<unsigned char> (s.size () + 1);

      // Set the exponent.
      //
      b[1] = exp;

      // Set the significant digits (big-endian byte order) and the terminator,
      // if any.
      //
      c = 2;
      while (!s.empty ())
      {
        b[i] = s.front ();
        s.pop_front ();
        ++c;
      }
    }
  }
}
