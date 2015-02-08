// file      : pgsql/types/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <vector>
#include <memory>  // std::auto_ptr
#include <cstring> // std::memcmp, std::memcpy, std::str[n]cmp, std::strlen
#include <cstddef> // std::size_t

#include <odb/core.hxx>

#include <common/buffer.hxx>

struct bitfield
{
  unsigned int a: 1;
  unsigned int b: 1;
  unsigned int c: 1;
  unsigned int d: 1;
};

inline bool
operator== (bitfield x, bitfield y)
{
  return
    x.a == y.a &&
    x.b == y.b &&
    x.c == y.c &&
    x.d == y.d;
}

struct varbit
{
  std::size_t size;
  ubuffer ubuffer_;

  bool
  compare (const varbit& x) const
  {
    if (size != x.size)
      return false;

    std::size_t byte_len = size / 8;

    if (std::memcmp (ubuffer_.data (), x.ubuffer_.data (), byte_len) != 0)
      return false;

    std::size_t trailing_bits = size % 8;

    if (trailing_bits != 0)
    {
      unsigned char mask (0xFFU << (8 - trailing_bits));

      return (ubuffer_.data ()[byte_len] & mask) ==
        (x.ubuffer_.data ()[byte_len] & mask);
    }

    return true;
  }
};

inline bool
operator== (const varbit& x, const varbit& y)
{
  return x.compare (y);
}

#pragma db value(bitfield) type ("BIT(4)")

typedef std::auto_ptr<std::string> string_ptr;

enum color {red, green, blue};

#pragma db object
struct object
{
  object () {}
  object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  // Integral types.
  //
  #pragma db type ("BOOL")
  bool bool_;

  #pragma db type ("SMALLINT")
  short short_;

  #pragma db type ("INT")
  int int_;

  #pragma db type ("BIGINT")
  long long long_long_;

  // Float types.
  //
  #pragma db type ("REAL")
  float float_;

  #pragma db type ("FLOAT(32)")
  double float8_;

  #pragma db type ("DOUBLE PRECISION")
  double double_;

  // Data-time types.
  //
  #pragma db type ("DATE")
  int date_;

  #pragma db type ("TIME")
  long long time_;

  #pragma db type ("TIMESTAMP")
  long long timestamp_;

  // String and binary types.
  //
  #pragma db type ("CHAR(128)")
  std::string char_;

  #pragma db type ("VARCHAR(256)")
  std::string varchar_;

  #pragma db type ("TEXT")
  std::string text_;

  #pragma db type ("BYTEA")
  std::vector<char> bytea_;

  #pragma db type ("VARBIT(1024)")
  varbit varbit_;

  // #pragma db type ("BIT(4)") - assigned by #pragma db value
  bitfield bit_;

  // Other types.
  //
  #pragma db type ("UUID")
  char uuid_[16];

  // Test ENUM representation.
  //
  color enum_;

  // Test NULL value.
  //
  #pragma db type ("TEXT") null
  string_ptr null_;

  bool
  operator== (const object& y) const
  {
    return
      id_ == y.id_ &&
      bool_ == y.bool_ &&
      short_ == y.short_ &&
      int_ == y.int_ &&
      long_long_ == y.long_long_ &&
      float_ == y.float_ &&
      float8_ == y.float8_ &&
      double_ == y.double_ &&
      date_ == y.date_ &&
      time_ == y.time_ &&
      timestamp_ == y.timestamp_ &&
      char_ == y.char_ &&
      varchar_ == y.varchar_ &&
      text_ == y.text_ &&
      bytea_ == y.bytea_ &&
      bit_ == y.bit_ &&
      varbit_ == y.varbit_ &&
      memcmp (uuid_, y.uuid_, 16) == 0 &&
      enum_ == y.enum_ &&
      ((null_.get () == 0 && y.null_.get () == 0) || *null_ == *y.null_);
  }
};

// Test char array.
//
#pragma db object
struct char_array
{
  char_array () {}
  char_array (unsigned long id, const char* s)
      : id_ (id)
  {
    std::memcpy (s1, s, std::strlen (s) + 1); // VC++ strncpy deprecation.
    std::memcpy (s2, s, std::strlen (s) + 1);
    s3[0] = c1 = *s;
  }

  #pragma db id
  unsigned long id_;

  char s1[17];

  #pragma db type("CHAR(16)")
  char s2[16];

  char s3[1];
  char c1;

  bool
  operator== (const char_array& y) const
  {
    return id_ == y.id_ &&
      std::strcmp (s1, y.s1) == 0 &&
      std::strncmp (s2, y.s2, sizeof (s2)) == 0 &&
      s3[0] == y.s3[0] &&
      c1 == y.c1;
  }
};

#endif // TEST_HXX
