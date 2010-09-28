// file      : mysql/types/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <memory>  // std::auto_ptr
#include <cstddef> // std::size_t
#include <cstring> // std::{memcmp,memcpy}

#include <odb/core.hxx>

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
             unsigned int sec)
      : negative (n),
        year (y),
        month (m),
        day (d),
        hour (h),
        minute (min),
        second (sec)
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
      second == y.second;
  }

  bool negative;
  unsigned int year;
  unsigned int month;
  unsigned int day;
  unsigned int hour;
  unsigned int minute;
  unsigned int second;
};

struct buffer
{
  ~buffer ()
  {
    delete[] data_;
  }

  buffer ()
      : data_ (0), size_ (0)
  {
  }

  buffer (const void* data, std::size_t size)
      : data_ (0), size_ (size)
  {
    data_ = new char[size_];
    std::memcpy (data_, data, size_);
  }

  buffer (const buffer& y)
      : data_ (0), size_ (0)
  {
    assign (y.data_, y.size_);
  }

  buffer&
  operator= (const buffer& y)
  {
    if (this != &y)
      assign (y.data_, y.size_);

    return *this;
  }

  void
  assign (const void* data, std::size_t size)
  {
    if (size_ < size)
    {
      char* p (new char[size]);
      delete[] data_;
      data_ = p;
    }

    std::memcpy (data_, data, size);
    size_ = size;
  }

  char*
  data ()
  {
    return data_;
  }

  const char*
  data () const
  {
    return data_;
  }

  std::size_t
  size () const
  {
    return size_;
  }

  bool
  operator== (const buffer& y) const
  {
    return size_ == y.size_ && std::memcmp (data_, y.data_, size_) == 0;
  }

private:
  char* data_;
  std::size_t size_;
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

#pragma db value(bitfield) type ("BIT(4) NOT NULL")

typedef std::set<std::string> set;
typedef std::auto_ptr<std::string> string_ptr;

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

  #pragma db type ("TINYINT NOT NULL")
  signed char schar_;

  #pragma db type ("TINYINT UNSIGNED NOT NULL")
  unsigned char uchar_;

  #pragma db type ("SMALLINT NOT NULL")
  short short_;

  #pragma db type ("SMALLINT UNSIGNED NOT NULL")
  unsigned short ushort_;

  #pragma db type ("MEDIUMINT NOT NULL")
  int mint_;

  #pragma db type ("MEDIUMINT UNSIGNED NOT NULL")
  unsigned int umint_;

  #pragma db type ("INT NOT NULL")
  int int_;

  #pragma db type ("INT UNSIGNED NOT NULL")
  unsigned int uint_;

  #pragma db type ("BIGINT NOT NULL")
  long long long_long_;

  #pragma db type ("BIGINT UNSIGNED NOT NULL")
  unsigned long long ulong_long_;

  // Float types.
  //
  #pragma db type ("FLOAT NOT NULL")
  float float_;

  #pragma db type ("DOUBLE NOT NULL")
  double double_;

  #pragma db type ("DECIMAL(6,3) NOT NULL")
  std::string decimal_;

  // Data-time types.
  //
  #pragma db type ("DATE NOT NULL")
  date_time date_;

  #pragma db type ("TIME NOT NULL")
  date_time time_;

  #pragma db type ("DATETIME NOT NULL")
  date_time date_time_;

  #pragma db type ("TIMESTAMP NOT NULL")
  date_time timestamp_;

  #pragma db type ("YEAR NOT NULL")
  short year_;

  // String and binary types.
  //
  #pragma db type ("CHAR(128) NOT NULL")
  std::string char_;

  #pragma db type ("BINARY(128) NOT NULL")
  buffer binary_;

  #pragma db type ("VARCHAR(256) NOT NULL")
  std::string varchar_;

  #pragma db type ("VARBINARY(256) NOT NULL")
  buffer varbinary_;

  #pragma db type ("TINYTEXT NOT NULL")
  std::string tinytext_;

  #pragma db type ("TINYBLOB NOT NULL")
  buffer tinyblob_;

  #pragma db type ("TEXT NOT NULL")
  std::string text_;

  #pragma db type ("BLOB NOT NULL")
  buffer blob_;

  #pragma db type ("MEDIUMTEXT NOT NULL")
  std::string mediumtext_;

  #pragma db type ("MEDIUMBLOB NOT NULL")
  buffer mediumblob_;

  #pragma db type ("LONGTEXT NOT NULL")
  std::string longtext_;

  #pragma db type ("LONGBLOB NOT NULL")
  buffer longblob_;

  // Other types.
  //
  // #pragma db type ("BIT(4) NOT NULL") - assigned by #pragma db value
  bitfield bit_;

  #pragma db type ("ENUM('red', 'green', 'blue') NOT NULL")
  std::string enum_;

  #pragma db type ("SET('red', 'green', 'blue') NOT NULL")
  set set_;

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
      enum_ == y.enum_ &&
      set_ == y.set_ &&
      ((null_.get () == 0 && y.null_.get () == 0) || *null_ == *y.null_);
  }
};

#endif // TEST_HXX
