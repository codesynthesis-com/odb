// file      : oracle/types/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <memory>   // std::auto_ptr
#include <cstring> // std::memcpy, std::str[n]cmp, std::strlen

#include <odb/core.hxx>

struct date_time
{
  date_time ()
  {
  }

  date_time (unsigned short y,
             unsigned char m,
             unsigned char d,
             unsigned char h,
             unsigned char min,
             unsigned char sec,
             unsigned int nsec)
      : year (y),
        month (m),
        day (d),
        hour (h),
        minute (min),
        second (sec),
        nanosecond (nsec)
  {
  }

  bool
  operator== (const date_time& y) const
  {
    return
      year == y.year &&
      month == y.month &&
      day == y.day &&
      hour == y.hour &&
      minute == y.minute &&
      second == y.second &&
      nanosecond == y.nanosecond;
  }

  unsigned short year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char minute;
  unsigned char second;
  unsigned int nanosecond;
};

struct time_interval
{
  time_interval ()
  {
  }

  time_interval (int y, int m, int d, int h, int min, int sec, int nsec)
      : year (y),
        month (m),
        day (d),
        hour (h),
        minute (min),
        second (sec),
        nanosecond (nsec)
  {
  }

  bool
  operator== (const time_interval& y) const
  {
    return
      year == y.year &&
      month == y.month &&
      day == y.day &&
      hour == y.hour &&
      minute == y.minute &&
      second == y.second &&
      nanosecond == y.nanosecond;
  }

  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  int nanosecond;
};

typedef std::auto_ptr<std::string> string_ptr;
typedef std::vector<std::string> strings;

#pragma db object
struct object
{
  object () {}
  object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned int id_;

  // Integral types.
  //
  #pragma db type ("NUMBER(10)")
  int int_;

  #pragma db type ("NUMBER(10)")
  unsigned uint_;

  #pragma db type ("NUMBER(19)")
  long long long_long_;

  #pragma db type ("NUMBER(20)")
  unsigned long long ulong_long_;

  // Float types.
  //
  #pragma db type ("FLOAT(24)")
  float float_;

  #pragma db type ("FLOAT(53)")
  double double_;

  #pragma db type ("NUMBER(7,3)")
  float num_float_;

  #pragma db type ("NUMBER(15,5)")
  double num_double_;

  #pragma db type ("BINARY_FLOAT")
  float binary_float_;

  #pragma db type ("BINARY_DOUBLE")
  double binary_double_;

  // Data-time types.
  //
  #pragma db type ("DATE")
  date_time date_;

  #pragma db type ("TIMESTAMP(6)")
  date_time timestamp_;

  #pragma db type ("INTERVAL DAY TO SECOND")
  time_interval interval_ds_;

  #pragma db type ("INTERVAL YEAR TO MONTH")
  time_interval interval_ym_;

  // String and binary types.
  //
  #pragma db type ("CHAR(13)")
  std::string char_;

  #pragma db type ("VARCHAR2(512)") null
  std::string varchar2_;

  #pragma db type ("NCHAR(8)")
  std::string nchar_;

  #pragma db type ("NVARCHAR2(512)") null
  std::string nvarchar2_;

  // Oracle treats empty and NULL VARCHAR2 the same. Test that we
  // handle this.
  //
  std::string empty_;
  std::vector<std::string> empty_c_;

  #pragma db type ("RAW(1024)")
  std::vector<char> raw_;

  // LOB types.
  //
  #pragma db type ("BLOB")
  std::vector<char> blob_;

  #pragma db type ("CLOB")
  std::string clob_;

  #pragma db type ("NCLOB")
  std::string nclob_;

  // Test containers of LOBs
  //
  #pragma db value_type ("CLOB")
  strings strs_;

  // Test NULL value.
  //
  #pragma db type ("VARCHAR2(32)") null
  string_ptr null_;

  bool
  operator== (const object& y) const
  {
    return
      id_ == y.id_ &&
      int_ == y.int_ &&
      uint_ == y.uint_ &&
      long_long_ == y.long_long_ &&
      ulong_long_ == y.ulong_long_ &&
      float_ == y.float_ &&
      double_ == y.double_ &&
      num_float_ == y.num_float_ &&
      num_double_ == y.num_double_ &&
      binary_float_ == y.binary_float_ &&
      binary_double_ == y.binary_double_ &&
      date_ == y.date_ &&
      timestamp_ == y.timestamp_ &&
      interval_ds_ == y.interval_ds_ &&
      interval_ym_ == y.interval_ym_ &&
      char_ == y.char_ &&
      varchar2_ == y.varchar2_ &&
      nchar_ == y.nchar_ &&
      nvarchar2_ == y.nvarchar2_ &&
      empty_ == y.empty_ &&
      empty_c_ == y.empty_c_ &&
      raw_ == y.raw_ &&
      blob_ == y.blob_ &&
      clob_ == y.clob_ &&
      nclob_ == y.nclob_ &&
      strs_ == y.strs_ &&
      ((null_.get () == 0 && y.null_.get () == 0) || *null_ == *y.null_);
  }
};

#pragma db object
struct big_uint
{
  big_uint (unsigned int id = 0, unsigned long long v = 0)
      : id_ (id), value (v)
  {
  }

  #pragma db id
  unsigned int id_;

  unsigned long long value;

  bool
  operator== (const big_uint& y) const
  {
    return id_ == y.id_ && value == y.value;
  }
};

#pragma db object
struct big_int
{
  big_int (unsigned int id = 0, long long v = 0)
      : id_ (id), value (v)
  {
  }

  #pragma db id
  unsigned int id_;

  long long value;

  bool
  operator== (const big_int& y) const
  {
    return id_ == y.id_ && value == y.value;
  }
};

#pragma db object
struct descriptor
{
  descriptor (unsigned int id = 0): id_ (id) {}

  #pragma db id
  unsigned int id_;

  #pragma db type ("BLOB")
  std::vector<char> blob;

  #pragma db type ("TIMESTAMP(6)")
  date_time timestamp;

  #pragma db type ("INTERVAL DAY TO SECOND")
  time_interval interval_ds;

  #pragma db type ("INTERVAL YEAR TO MONTH")
  time_interval interval_ym;

  bool
  operator== (const descriptor& y) const
  {
    return id_ == y.id_ &&
      blob == y.blob &&
      timestamp == y.timestamp &&
      interval_ds == y.interval_ds &&
      interval_ym == y.interval_ym;
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

  #pragma db type("CHAR(16)") null
  char s2[16];

  #pragma db null
  char s3[1];

  #pragma db null
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
