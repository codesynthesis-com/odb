// file      : mssql/types/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h> // GUID
#elif defined(HOST_WIN32)
typedef struct _GUID
{
   unsigned int Data1;
   unsigned short Data2;
   unsigned short Data3;
   unsigned char Data4[8];
} GUID;
#endif

#include <string>
#include <vector>
#include <memory>   // std::auto_ptr
#include <cstring>  // std::memcmp, std::memcpy, std::str[n]cmp, std::strlen
#include <cwchar>   // std::wcslen, std::wcs[n]cmp

#include <odb/core.hxx>

struct date_time
{
  date_time ()
  {
  }

  date_time (short y,
             unsigned short m,
             unsigned short d,
             unsigned short h,
             unsigned short min,
             unsigned short sec,
             unsigned int f,
             short tzh,
             short tzm)
      : year (y),
        month (m),
        day (d),
        hour (h),
        minute (min),
        second (sec),
        fraction (f),
        timezone_hour (tzh),
        timezone_minute (tzm)
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
      fraction == y.fraction &&
      timezone_hour == y.timezone_hour &&
      timezone_minute == y.timezone_minute;
  }

  short year;
  unsigned short month;
  unsigned short day;
  unsigned short hour;
  unsigned short minute;
  unsigned short second;
  unsigned int fraction;
  short timezone_hour;
  short timezone_minute;
};

#pragma db object
struct object
{
  object () {}
  object (unsigned int id): id_ (id) {}

  #pragma db id
  unsigned int id_;

  // Integer types.
  //
  #pragma db type ("BIT")
  unsigned char bit_;

  #pragma db type ("TINYINT")
  unsigned char utint_;

  #pragma db type ("TINYINT")
  unsigned char stint_;

  #pragma db type ("SMALLINT")
  unsigned short usint_;

  #pragma db type ("SMALLINT")
  short ssint_;

  #pragma db type ("INT")
  unsigned int uint_;

  #pragma db type ("INTEGER")
  int sint_;

  #pragma db type ("BIGINT")
  unsigned long long ubint_;

  #pragma db type ("BIGINT")
  long long sbint_;

  // Floating/fixed point types.
  //
  #pragma db type ("SMALLMONEY")
  float fsm_;

  #pragma db type ("SMALLMONEY")
  double dsm_;

  #pragma db type ("SMALLMONEY")
  int ism_;

  #pragma db type ("MONEY")
  double dm1_;

  #pragma db type ("MONEY")
  double dm2_;

  #pragma db type ("MONEY")
  long long im_;

  #pragma db type ("REAL")
  float f4_;

  #pragma db type ("FLOAT")
  double f8_;

  // Strings.
  //
  #pragma db type ("CHAR(20)")
  std::string schar_;

  #pragma db type ("VARCHAR(128)")
  std::string svchar_;

  #pragma db type ("CHAR(1025)")
  std::string lchar_;

  #pragma db type ("CHARACTER VARYING(8000)")
  std::string lvchar_;

  #pragma db type ("VARCHAR(max)")
  std::string mvchar_;

  #pragma db type ("TEXT")
  std::string text_;

  // National strings.
  //
  #pragma db type ("NCHAR(20)")
  std::wstring snchar_;

  #pragma db type ("NVARCHAR(128)")
  std::wstring snvchar_;

  #pragma db type ("NCHAR(513)")
  std::wstring lnchar_;

  #pragma db type ("NATIONAL CHARACTER VARYING(4000)")
  std::wstring lnvchar_;

  #pragma db type ("NVARCHAR(max)")
  std::wstring mnvchar_;

  #pragma db type ("NTEXT")
  std::wstring ntext_;

  // Binary.
  //
  #pragma db type ("BINARY(9)")
  unsigned char sbin_[9];

  #pragma db type ("VARBINARY(256)")
  std::vector<char> svbin_;

  #pragma db type ("BINARY(1025)")
  char lbin_[1025];

  #pragma db type ("BINARY VARYING(8000)")
  std::vector<char> lvbin_;

  #pragma db type ("VARBINARY(max)")
  std::vector<unsigned char> mvbin_;

  #pragma db type ("IMAGE")
  std::vector<char> image_;

  // Date-time. SQL Server 2005 (9.0) only has DATETIME and SMALLDATETIME.
  //
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  #pragma db type ("DATE")
  date_time date_;

  #pragma db type ("TIME")
  date_time time7_;

  #pragma db type ("TIME(4)")
  date_time time4_;
#endif

  #pragma db type ("SMALLDATETIME")
  date_time sdt_;

  #pragma db type ("DATETIME")
  date_time dt_;

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  #pragma db type ("DATETIME2")
  date_time dt2_;

  #pragma db type ("DATETIMEOFFSET")
  date_time dto7_;

  #pragma db type ("DATETIMEOFFSET(0)")
  date_time dto0_;
#endif

  // Other types.
  //
#if defined(_WIN32) || defined(HOST_WIN32)
  //#pragma db type ("UNIQUEIDENTIFIER")
  GUID guid_;
#endif

  #pragma db type ("UNIQUEIDENTIFIER")
  char uuid_[16];

  bool
  operator== (const object& y) const
  {
    return
      id_ == y.id_ &&
      bit_ == y.bit_ &&
      utint_ == y.utint_ &&
      stint_ == y.stint_ &&
      usint_ == y.usint_ &&
      ssint_ == y.ssint_ &&
      uint_ == y.uint_ &&
      sint_ == y.sint_ &&
      ubint_ == y.ubint_ &&
      sbint_ == y.sbint_ &&
      fsm_ == y.fsm_ &&
      dsm_ == y.dsm_ &&
      ism_ == y.ism_ &&
      dm1_ == y.dm1_ &&
      dm2_ == y.dm2_ &&
      im_ == y.im_ &&
      f4_ == y.f4_ &&
      f8_ == y.f8_ &&

      schar_ == y.schar_ &&
      svchar_ == y.svchar_ &&
      lchar_ == y.lchar_ &&
      lvchar_ == y.lvchar_ &&
      mvchar_ == y.mvchar_ &&
      text_ == y.text_ &&

      snchar_ == y.snchar_ &&
      snvchar_ == y.snvchar_ &&
      lnchar_ == y.lnchar_ &&
      lnvchar_ == y.lnvchar_ &&
      mnvchar_ == y.mnvchar_ &&
      ntext_ == y.ntext_ &&

      std::memcmp (sbin_, y.sbin_, sizeof (sbin_)) == 0 &&
      svbin_ == y.svbin_ &&
      std::memcmp (lbin_, y.lbin_, sizeof (lbin_)) == 0 &&
      lvbin_ == y.lvbin_ &&
      mvbin_ == y.mvbin_ &&
      image_ == y.image_

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && date_ == y.date_
      && time7_ == y.time7_
      && time4_ == y.time4_
#endif
      && sdt_ == y.sdt_
      && dt_ == y.dt_
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && dt2_ == y.dt2_
      && dto7_ == y.dto7_
      && dto0_ == y.dto0_
#endif

#ifdef _WIN32
      && std::memcmp (&guid_, &y.guid_, sizeof (guid_)) == 0
#endif
      && std::memcmp (uuid_, y.uuid_, sizeof (uuid_)) == 0;
  }
};

// Test long NULL data.
//
#pragma db object
struct long_null
{
  long_null () {}
  long_null (unsigned int id): id_ (id) {}

  #pragma db id
  unsigned int id_;

  #pragma db type ("VARCHAR(max)") null
  std::auto_ptr<std::string> str_;

  bool
  operator== (const long_null& y) const
  {
    return
      id_ == y.id_ &&
      ((str_.get () == 0 && y.str_.get () == 0) || *str_ == *y.str_);
  }
};

// Test long data in containers, in particular column re-arrangement.
//
#pragma db value
struct long_comp
{
  long_comp () {}
  long_comp (std::string s, unsigned int n): str (s), num (n) {}

  #pragma db type ("VARCHAR(max)")
  std::string str;

  unsigned int num;

  bool
  operator== (const long_comp& y) const
  {
    return str == y.str && num == y.num;
  }
};

#pragma db object
struct long_cont
{
  long_cont () {}
  long_cont (unsigned int id): id_ (id) {}

  #pragma db id
  unsigned int id_;

  std::vector<long_comp> v;

  bool
  operator== (const long_cont& y) const
  {
    return id_ == y.id_ && v == y.v;
  }
};

// Test char/wchar_t arrays.
//
#pragma db object
struct char_array
{
  char_array () {}
  char_array (unsigned long id, const char* s, const wchar_t* ws)
      : id_ (id)
  {
    std::memcpy (s1, s, std::strlen (s) + 1); // VC++ strncpy deprecation.
    std::memcpy (s2, s, std::strlen (s) + 1);
    s3[0] = c1 = *s;

    std::memcpy (ws1, ws, (std::wcslen (ws) + 1) * sizeof (wchar_t));
    std::memcpy (ws2, ws, (std::wcslen (ws) + 1) * sizeof (wchar_t));
    ws3[0] = wc1 = *ws;

    if (std::strlen (s) == sizeof (s2))
    {
      std::memset (ls1, '1', 1025);
      ls1[1025] = '\0';
      std::memset (ls2, '2', 1025);

      for (std::size_t i (0); i < 257; ++i)
      {
        lws1[i] = L'1';
        lws2[i] = L'2';
      }
      lws1[257] = L'\0';
    }
    else
    {
      std::memcpy (ls1, s, std::strlen (s) + 1); // VC++ strcpy deprecation.
      std::memcpy (ls2, s, std::strlen (s) + 1);

      std::memcpy (lws1, ws, (std::wcslen (ws) + 1) * sizeof (wchar_t));
      std::memcpy (lws2, ws, (std::wcslen (ws) + 1) * sizeof (wchar_t));
    }
  }

  #pragma db id
  unsigned long id_;

  //
  //
  char s1[17];

  #pragma db type("CHAR(16)")
  char s2[16];

  char s3[1];
  char c1;

  // Long data.
  //
  char ls1[1026];

  #pragma db type("CHAR(1025)")
  char ls2[1025];

  //
  //
  wchar_t ws1[17];

  #pragma db type("NCHAR(16)")
  wchar_t ws2[16];

  wchar_t ws3[1];
  wchar_t wc1;

  // Long data.
  //
  wchar_t lws1[258];

  #pragma db type("NCHAR(257)")
  wchar_t lws2[257];

  bool
  operator== (const char_array& y) const
  {
    return id_ == y.id_ &&

      std::strcmp (s1, y.s1) == 0 &&
      std::strncmp (s2, y.s2, sizeof (s2)) == 0 &&
      s3[0] == y.s3[0] &&
      c1 == y.c1 &&

      std::strcmp (ls1, y.ls1) == 0 &&
      std::strncmp (ls2, y.ls2, sizeof (ls2)) == 0 &&

      std::wcscmp (ws1, y.ws1) == 0 &&
      std::wcsncmp (ws2, y.ws2, sizeof (ws2) / sizeof (wchar_t)) == 0 &&
      ws3[0] == y.ws3[0] &&
      wc1 == y.wc1 &&

      std::wcscmp (lws1, y.lws1) == 0 &&
      std::wcsncmp (lws2, y.lws2, sizeof (lws2) / sizeof (wchar_t)) == 0;
  }
};

// Test optimistic concurrency using ROWVERSION, both with auto and
// manually-assigned ids.
//
#pragma db object optimistic
struct rowversion
{
  rowversion (unsigned int id = 0): id_ (id), ver (0) {}

  #pragma db id
  unsigned int id_;

  #pragma db version type("ROWVERSION")
#ifdef _WIN32
  unsigned __int64 ver;
#else
  unsigned long long ver;
#endif

  std::string str;
};

#pragma db object optimistic
struct rowversion_auto
{
  rowversion_auto (): ver (0) {}

  #pragma db id auto
  unsigned int id_;

  #pragma db version type("ROWVERSION")
#ifdef _WIN32
  unsigned __int64 ver;
#else
  unsigned long long ver;
#endif

  std::string str;
};

#endif // TEST_HXX
