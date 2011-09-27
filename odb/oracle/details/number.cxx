// file      : odb/oracle/details/number.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstddef>  // std::size_t
#include <cassert>

#include <odb/oracle/details/number.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    namespace details
    {
      // The VARNUM type's binary representation is made up of the following
      // bit fields, ordered in increasing memory address.
      //
      // 000 to 007: The size of the binary representation, including the
      //             exponent bits and the mantissa bits, but excluding these
      //             length bits.
      //
      // 008 to 015: The base-100 exponent bits. The most significant bit is
      //             the sign bit of the number, which is set for positive
      //             numbers and cleared for negative numbers. For positive
      //             numbers, the exponent has a bias of 65 added to it.
      //
      // 016 to 175: The mantissa bits. Each byte of this field represents a
      //             single base-100 value.
      //
      //

      long long
      number_to_int64 (const char* b)
      {
        // All bytes in the buffer are interpreted as unsigned.
        //
        const unsigned char* ub (reinterpret_cast<const unsigned char*> (b));

        int n (ub[0]);

        // Zero is represented by zero significant bits and an exponent
        // set to 128.
        //
        if (n == 1)
        {
          assert (ub[1] == 128);
          return 0;
        }

        long long v (0);

        // Test the sign bit of the exponent.
        //
        if ((ub[1] & 0x80) != 0)
        {
          // The unbiased exponent of a positive number may be calculated as
          // ub[1] - 128 - 65. For an integer, this is the order of magnitude
          // of the number. Calculate the maximum weight, 100 ^ o, where o is
          // the order of magnitude of the number.
          //
          long long w (1);

          for (size_t i (0), o (ub[1] - 193); i < o; ++i)
            w *= 100;

          // Accumlate the sum of the significant base-100 terms.
          //
          for (const unsigned char* m (ub + 2), *e (ub + 1 + n); m < e; ++m)
          {
            v += (*m - 1) * w;
            w /= 100;
          }
        }
        else
        {
          // The unbiased exponent of a negative number is calculated as
          // (~ub[1] & 0x7F) - 193. For an integer, this is the order of
          // magnitude of the number. Calculate the maximum weight, 100 ^ o,
          // where o is the order of magnitude of the number.
          //
          long long w (1);

          for (size_t i (0), o ((~ub[1] & 0x7F) - 65); i < o; ++i)
            w *= 100;

          // Accumulate the sum of the significant base-100 terms. Note that
          // negative values will have a terminator byte which is included
          // in the length. This is ignored.
          //
          for (const unsigned char* m (ub + 2), *e (ub + n); m < e; ++m)
          {
            v -= (101 - *m) * w;
            w /= 100;
          }
        }

        return v;
      }

      void
      int64_to_number (char* b, long long v)
      {
        // We assume that b is long enough to contain a long long VARNUM
        // representation, that being 12 bytes.
        //

        // All bytes in the buffer are interpreted as unsigned.
        //
        unsigned char* ub (reinterpret_cast<unsigned char*> (b));

        if (v == 0)
        {
          ub[0] = 1;
          ub[1] = 128;

          return;
        }

        bool sig (false);
        size_t n (0);
        unsigned char t[11], *m (t);

        if (v < 0)
        {
          // Termination marker for negative numbers.
          //
          *m++ = 102;

          while (v != 0)
          {
            int r (static_cast<int> (v % 100));
            sig = sig || r != 0;

            if (sig)
              *m++ = static_cast<unsigned char> (101 + r);

            v /= 100;
            ++n;
          }

          // The exponent is one less than the number of base 100 digits. It is
          // inverted for negative values.
          //
          ub[1] = static_cast<unsigned char> (~(n + 192));
        }
        else
        {
          while (v != 0)
          {
            int r (static_cast<int> (v % 100));
            sig = sig || r != 0;

            if (sig)
              *m++ = static_cast<unsigned char> (r + 1);

            v /= 100;
            ++n;
          }

          // Exponent is one less than the number of base 100 digits.
          //
          ub[1] = static_cast<unsigned char> (n + 192);
        }

        // Set the length.
        //
        ub[0] = static_cast<unsigned char> (m - t + 1);

        // Set the significant digits in big-endian byte order and the
        // terminator, if any.
        //
        for (size_t i (2); m > t; ++i)
          ub[i] = *--m;
      }

      unsigned long long
      number_to_uint64 (const char* b)
      {
        // All bytes in the buffer are interpreted as unsigned.
        //
        const unsigned char* ub (reinterpret_cast<const unsigned char*> (b));

        int n (ub[0]);

        // Zero is represented by zero significant bits and an exponent
        // set to 128.
        //
        if (n == 1)
        {
          assert (ub[1] == 128);
          return 0;
        }

        unsigned long long v (0);

        // Test the sign bit of the exponent.
        //
        if (ub[1] & 0x80)
        {
          assert (false);
          return 0;
        }

        // The unbiased exponent of a positive number may be calculated as
        // ub[1] - 128 - 65. For an integer, this is the order of magnitude
        // of the number. Calculate the maximum weight, 100 ^ o, where o is
        // the order of magnitude of the number.
        //
        unsigned long long w (1);

        for (size_t i (0), o (ub[1] - 193); i < o; ++i)
          w *= 100;

        // Accumlate the sum of the significant base-100 terms.
        //
        for (const unsigned char* m (ub + 2), *e (ub + 1 + n); m < e; ++m)
        {
          v += (*m - 1) * w;
          w /= 100;
        }

        return v;
      }

      void
      uint64_to_number (char* b, unsigned long long v)
      {
        // We assume that b is long enough to contain an unsigned long long
        // VARNUM representation, that being 12 bytes.
        //

        // All bytes in the buffer are interpreted as unsigned.
        //
        unsigned char* ub (reinterpret_cast<unsigned char*> (b));

        if (v == 0)
        {
          ub[0] = 1;
          ub[1] = 128;

          return;
        }

        bool sig (false);
        size_t n (0);
        unsigned char t[11], *m (t);

        while (v != 0)
        {
          int r (static_cast<int> (v % 100));
          sig = sig || r != 0;

          if (sig)
            *m++ = static_cast<unsigned char> (r + 1);

          v /= 100;
          ++n;
        }

        // Exponent is one less than the number of base 100 digits.
        //
        ub[1] = static_cast<unsigned char> (n + 192);

        // Set the length.
        //
        ub[0] = static_cast<unsigned char> (m - t + 1);

        // Set the significant digits in big-endian byte order.
        //
        for (size_t i (2); m > t; ++i)
          ub[i] = *--m;
      }
    }
  }
}
