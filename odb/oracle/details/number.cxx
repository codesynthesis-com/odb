// file      : odb/oracle/details/number.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
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
      // The NUMBER type's binary representation is made up of the following
      // bit fields, ordered in increasing memory address.
      //
      // 000 to 007: The base-100 exponent bits. The most significant bit is
      //             the sign bit of the number, which is set for positive
      //             numbers and cleared for negative numbers. For positive
      //             numbers, the exponent has a bias of 65 added to it.
      //
      // 008 to 167: The mantissa bits. Each byte of this field represents a
      //             single base-100 value.
      //
      //

      long long
      number_to_int64 (const char* b, size_t n)
      {
        // All bytes in the buffer are interpreted as unsigned.
        //
        const unsigned char* ub (reinterpret_cast<const unsigned char*> (b));

        // Zero is represented by zero significant bits and an exponent
        // set to 128.
        //
        if (n == 1)
        {
          assert (ub[0] == 128);
          return 0;
        }

        long long v (0);

        // Test the sign bit of the exponent.
        //
        if ((ub[0] & 0x80) != 0)
        {
          // The unbiased exponent of a positive number may be calculated as
          // ub[0 - 128 - 65. For an integer, this is the order of magnitude
          // of the number. Calculate the maximum weight, 100 ^ o, where o is
          // the order of magnitude of the number.
          //
          long long w (1);

          for (size_t i (0), o (ub[0] - 193); i < o; ++i)
            w *= 100;

          // Accumlate the sum of the significant base-100 terms.
          //
          for (const unsigned char* m (ub + 1), *e (ub + n); m < e; ++m)
          {
            v += (*m - 1) * w;
            w /= 100;
          }
        }
        else
        {
          // The unbiased exponent of a negative number is calculated as
          // (~ub[0] & 0x7F) - 193. For an integer, this is the order of
          // magnitude of the number. Calculate the maximum weight, 100 ^ o,
          // where o is the order of magnitude of the number.
          //
          long long w (1);

          for (size_t i (0), o ((~ub[0] & 0x7F) - 65); i < o; ++i)
            w *= 100;

          // Accumulate the sum of the significant base-100 terms. Note that
          // negative values will have a terminator byte which is included
          // in the length. This is ignored.
          //
          for (const unsigned char* m (ub + 1), *e (ub + n - 1); m < e; ++m)
          {
            v -= (101 - *m) * w;
            w /= 100;
          }
        }

        return v;
      }

      void
      int64_to_number (char* b, size_t& n, long long v)
      {
        // We assume that b is long enough to contain a long long NUMBER
        // representation, that being 12 bytes.
        //

        // All bytes in the buffer are interpreted as unsigned.
        //
        unsigned char* ub (reinterpret_cast<unsigned char*> (b));

        if (v == 0)
        {
          ub[0] = 128;
          n = 1;

          return;
        }

        bool sig (false);
        unsigned char t[11], *m (t);
        n = 0;

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
          ub[0] = static_cast<unsigned char> (~(n + 192));
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
          ub[0] = static_cast<unsigned char> (n + 192);
        }

        // Set the length.
        //
        n = static_cast<unsigned char> (m - t + 1);

        // Set the significant digits in big-endian byte order and the
        // terminator, if any.
        //
        for (size_t i (1); m > t; ++i)
          ub[i] = *--m;
      }

      unsigned long long
      number_to_uint64 (const char* b, size_t n)
      {
        // All bytes in the buffer are interpreted as unsigned.
        //
        const unsigned char* ub (reinterpret_cast<const unsigned char*> (b));

        // Zero is represented by zero significant bits and an exponent
        // set to 128.
        //
        if (n == 1)
        {
          assert (ub[0] == 128);
          return 0;
        }

        unsigned long long v (0);

        // Test the sign bit of the exponent.
        //
        if ((ub[0] & 0x80) == 0)
        {
          assert (false);
          return 0;
        }

        // The unbiased exponent of a positive number may be calculated as
        // ub[0] - 128 - 65. For an integer, this is the order of magnitude
        // of the number. Calculate the maximum weight, 100 ^ o, where o is
        // the order of magnitude of the number.
        //
        unsigned long long w (1);

        for (size_t i (0), o (ub[0] - 193); i < o; ++i)
          w *= 100;

        // Accumlate the sum of the significant base-100 terms.
        //
        for (const unsigned char* m (ub + 1), *e (ub + n); m < e; ++m)
        {
          v += (*m - 1) * w;
          w /= 100;
        }

        return v;
      }

      void
      uint64_to_number (char* b, size_t& n, unsigned long long v)
      {
        // We assume that b is long enough to contain an unsigned long long
        // NUMBER representation, that being 12 bytes.
        //

        // All bytes in the buffer are interpreted as unsigned.
        //
        unsigned char* ub (reinterpret_cast<unsigned char*> (b));

        if (v == 0)
        {
          ub[0] = 128;
          n = 1;

          return;
        }

        bool sig (false);
        unsigned char t[11], *m (t);
        n = 0;

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
        ub[0] = static_cast<unsigned char> (n + 192);

        // Set the length.
        //
        n = static_cast<unsigned char> (m - t + 1);

        // Set the significant digits in big-endian byte order.
        //
        for (size_t i (1); m > t; ++i)
          ub[i] = *--m;
      }
    }
  }
}
