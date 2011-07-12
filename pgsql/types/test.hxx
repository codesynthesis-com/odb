// file      : pgsql/types/test.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <memory>  // std::auto_ptr
#include <cstring> // std::memcmp

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

#pragma db value(bitfield) type ("BIT(4) NOT NULL")

typedef std::auto_ptr<std::string> string_ptr;

enum color {red, green, blue};

#pragma db object
struct object
{
  object (unsigned long id)
      : id_ (id)
  {
  }

  object ()
  {
  }

  #pragma db id
  unsigned long id_;

  // Integral types.
  //
  #pragma db type ("BOOL NOT NULL")
  bool bool_;

  #pragma db type ("SMALLINT NOT NULL")
  short short_;

  #pragma db type ("INT NOT NULL")
  int int_;

  #pragma db type ("BIGINT NOT NULL")
  long long long_long_;

  // Float types.
  //
  #pragma db type ("REAL NOT NULL")
  float float_;

  #pragma db type ("FLOAT(32) NOT NULL")
  double float8_;

  #pragma db type ("DOUBLE PRECISION NOT NULL")
  double double_;

  // #pragma db type ("NUMERIC(6,3) NOT NULL")
  // std::string numeric_;

  // Data-time types.
  //
  #pragma db type ("DATE NOT NULL")
  int date_;

  #pragma db type ("TIME NOT NULL")
  long long time_;

  #pragma db type ("TIMESTAMP NOT NULL")
  long long timestamp_;

  // String and binary types.
  //
  #pragma db type ("CHAR(128) NOT NULL")
  std::string char_;

  #pragma db type ("VARCHAR(256) NOT NULL")
  std::string varchar_;

  #pragma db type ("TEXT NOT NULL")
  std::string text_;

  #pragma db type ("BYTEA NOT NULL")
  buffer bytea_;

  #pragma db type ("VARBIT(1024) NOT NULL")
  buffer varbit_;

  // #pragma db type ("BIT(4) NOT NULL") - assigned by #pragma db value
  bitfield bit_;

  // Other types.
  //
  #pragma db type ("UUID NOT NULL")
  unsigned char uuid_[16];

  // Test ENUM representation.
  //
  color enum_;

  // Test NULL value.
  //
  #pragma db type ("TEXT")
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
      // numeric__ == y.numeric_ &&
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

#endif // TEST_HXX
