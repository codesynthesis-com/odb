// file      : sqlite/types/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <string>
#include <vector>
#include <memory>  // std::auto_ptr
#include <cstring> // std::memcpy, std::str[n]cmp, std::strlen

#ifdef _WIN32
#  include <cwchar> // std::wcslen, std::wcs[n]cmp
#endif

#include <odb/core.hxx>

typedef std::auto_ptr<std::string> string_ptr;

#pragma db object
struct object
{
  object () {}
  object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  #pragma db type("BOOL")
  bool bool_;

  #pragma db type("INTEGER")
  int integer_;

  #pragma db type("REAL")
  double real_;

  double nan_; // Represented in SQLite as NULL.

  #pragma db type("TEXT")
  std::string text_;

#ifdef _WIN32
  std::wstring wtext_;
#endif

  #pragma db type("BLOB")
  std::vector<char> blob_;

  // Test NULL value.
  //
  #pragma db type("TEXT") null
  string_ptr null_;

  bool
  operator== (const object& y) const
  {
    return id_ == y.id_
      && bool_ == y.bool_
      && integer_ == y.integer_
      && real_ == y.real_
      && nan_ != nan_
      && text_ == y.text_
#ifdef _WIN32
      && wtext_ == y.wtext_
#endif
      && blob_ == y.blob_
      && ((null_.get () == 0 && y.null_.get () == 0) || *null_ == *y.null_);
  }
};

// Test char/wchar_t arrays.
//
#pragma db object
struct char_array
{
  char_array () {}
  char_array (unsigned long id
              , const char* s
#ifdef _WIN32
              , const wchar_t* ws
#endif
  )
      : id_ (id)
  {
    std::memcpy (s1, s, std::strlen (s) + 1); // VC++ strncpy deprecation.
    s2[0] = c1 = *s;

#ifdef _WIN32
    std::memcpy (ws1, ws, (std::wcslen (ws) + 1) * sizeof (wchar_t));
    ws2[0] = wc1 = *ws;
#endif
  }

  #pragma db id
  unsigned long id_;

  char s1[17];
  char s2[1];
  char c1;

#ifdef _WIN32
  wchar_t ws1[17];
  wchar_t ws2[1];
  wchar_t wc1;
#endif

  bool
  operator== (const char_array& y) const
  {
    return id_ == y.id_
      && std::strncmp (s1, y.s1, sizeof (s1)) == 0
      && s2[0] == y.s2[0]
      && c1 == y.c1
#ifdef _WIN32
      && std::wcsncmp (ws1, y.ws1, sizeof (ws1) / 2) == 0
      && ws2[0] == y.ws2[0]
      && wc1 == y.wc1
#endif
      ;
  }
};

#endif // TEST_HXX
