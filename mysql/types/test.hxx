// file      : mysql/types/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <vector>
#include <memory>  // std::auto_ptr
#include <cstring> // std::memcpy, std::str[n]cmp, std::strlen

#include <odb/core.hxx>

typedef std::vector<char> buffer;

struct date_time
{
  date_time ()
  {
  }

  date_time (bool n,
             unsigned int y,
             unsigned int m,
             unsigned int d,
             unsigned int h,
             unsigned int min,
             unsigned int sec,
             unsigned int msec = 0)
      : negative (n),
        year (y),
        month (m),
        day (d),
        hour (h),
        minute (min),
        second (sec),
        microseconds (msec)
  {
  }

  bool
  operator== (const date_time& y) const
  {
    return
      negative == y.negative &&
      year == y.year &&
      month == y.month &&
      day == y.day &&
      hour == y.hour &&
      minute == y.minute &&
      second == y.second &&
      microseconds == y.microseconds;
  }

  bool negative;
  unsigned int year;
  unsigned int month;
  unsigned int day;
  unsigned int hour;
  unsigned int minute;
  unsigned int second;
  unsigned int microseconds;
};

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

#pragma db value(bitfield) type ("BIT(4)")

typedef std::set<std::string> set;
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

  #pragma db type ("TINYINT")
  signed char schar_;

  #pragma db type ("TINYINT UNSIGNED")
  unsigned char uchar_;

  #pragma db type ("SMALLINT")
  short short_;

  #pragma db type ("SMALLINT UNSIGNED")
  unsigned short ushort_;

  #pragma db type ("MEDIUMINT")
  int mint_;

  #pragma db type ("MEDIUMINT UNSIGNED")
  unsigned int umint_;

  #pragma db type ("INT")
  int int_;

  #pragma db type ("INT UNSIGNED")
  unsigned int uint_;

  #pragma db type ("BIGINT")
  long long long_long_;

  #pragma db type ("BIGINT UNSIGNED")
  unsigned long long ulong_long_;

  // Float types.
  //
  #pragma db type ("FLOAT")
  float float_;

  #pragma db type ("FLOAT(32)")
  double float8_;

  #pragma db type ("DOUBLE")
  double double_;

  #pragma db type ("DECIMAL(6,3)")
  std::string decimal_;

  // Data-time types.
  //
  #pragma db type ("DATE")
  date_time date_;

  #pragma db type ("TIME")
  date_time time_;

  #pragma db type ("DATETIME")
  date_time date_time_;

  #pragma db type ("TIMESTAMP")
  date_time timestamp_;

  #pragma db type ("YEAR")
  short year_;

  // String and binary types.
  //
  #pragma db type ("CHAR(128)")
  std::string char_;

  #pragma db type ("BINARY(128)")
  buffer binary_;

  #pragma db type ("VARCHAR(256)")
  std::string varchar_;

  #pragma db type ("VARBINARY(256)")
  buffer varbinary_;

  #pragma db type ("TINYTEXT")
  std::string tinytext_;

  #pragma db type ("TINYBLOB")
  buffer tinyblob_;

  #pragma db type ("TEXT")
  std::string text_;

  #pragma db type ("BLOB")
  buffer blob_;

  #pragma db type ("MEDIUMTEXT")
  std::string mediumtext_;

  #pragma db type ("MEDIUMBLOB")
  buffer mediumblob_;

  #pragma db type ("LONGTEXT")
  std::string longtext_;

  #pragma db type ("LONGBLOB")
  buffer longblob_;

  // Other types.
  //
  // #pragma db type ("BIT(4)") - assigned by #pragma db value
  bitfield bit_;

  // Test ENUM representations (integer and string).
  //
  color enum_def_;

  // Map to a custom MySQL ENUM type.
  //
  #pragma db type ("ENUM('R', 'G', 'B')")
  color enum_cst_;

  #pragma db type ("ENUM('red', 'green', 'blue')")
  std::string enum_str_;

  #pragma db type ("SET('red', 'green', 'blue')")
  set set_;

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
      schar_ == y.schar_ &&
      uchar_ == y.uchar_ &&
      short_ == y.short_ &&
      ushort_ == y.ushort_ &&
      mint_ == y.mint_ &&
      umint_ == y.umint_ &&
      int_ == y.int_ &&
      uint_ == y.uint_ &&
      long_long_ == y.long_long_ &&
      ulong_long_ == y.ulong_long_ &&
      float_ == y.float_ &&
      float8_ == y.float8_ &&
      double_ == y.double_ &&
      decimal_ == y.decimal_ &&
      date_ == y.date_ &&
      time_ == y.time_ &&
      date_time_ == y.date_time_ &&
      timestamp_ == y.timestamp_ &&
      year_ == y.year_ &&
      char_ == y.char_ &&
      binary_ == y.binary_ &&
      varchar_ == y.varchar_ &&
      varbinary_ == y.varbinary_ &&
      tinytext_ == y.tinytext_ &&
      tinyblob_ == y.tinyblob_ &&
      text_ == y.text_ &&
      blob_ == y.blob_ &&
      mediumtext_ == y.mediumtext_ &&
      mediumblob_ == y.mediumblob_ &&
      longtext_ == y.longtext_ &&
      longblob_ == y.longblob_ &&
      bit_ == y.bit_ &&
      enum_def_ == y.enum_def_ &&
      enum_cst_ == y.enum_cst_ &&
      enum_str_ == y.enum_str_ &&
      set_ == y.set_ &&
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

// MySQL server version view.
//
#pragma db view query(                                                  \
  "SELECT "                                                             \
  "CAST(SUBSTRING_INDEX(@@version, '.', 1) AS UNSIGNED),"               \
  "CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(@@version, '.', 2), '.', -1) AS UNSIGNED)," \
  "CAST(SUBSTRING_INDEX(SUBSTRING_INDEX(@@version, '-', 1), '.', -1) AS UNSIGNED)," \
  "@@protocol_version")
struct mysql_version
{
  unsigned int major;
  unsigned int minor;
  unsigned int release;

  unsigned int protocol;
};

#endif // TEST_HXX
