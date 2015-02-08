// file      : common/query/array/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <cstring> // std::memcpy, std::strlen

#ifdef HAVE_CXX11
#  include <array>
#endif

#include <odb/core.hxx>

#pragma db object
struct object
{
  object () {}
  object (unsigned long id, const char* s, const char* b)
      : id_ (id)
  {
    std::memcpy (s_, s, std::strlen (s) + 1); // VC++ strncpy deprecation.
    std::memcpy (s1_, s, std::strlen (s) + 1);

#ifdef HAVE_CXX11
    std::memcpy (a_.data (), s, std::strlen (s) + 1);
#endif
    c_ = c1_ = *s;
    std::memcpy (b_, b, sizeof (b_));
  }

  #pragma db id
  unsigned long id_;

  char s_[17];
  char s1_[17];

#ifdef HAVE_CXX11
#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_MYSQL)  || \
      defined(ODB_DATABASE_PGSQL)  || \
      defined(ODB_DATABASE_ORACLE) || \
      defined(ODB_DATABASE_MSSQL)
#    pragma db type("VARCHAR(16)")
#  elif defined(ODB_DATABASE_SQLITE)
#    pragma db type("TEXT")
#  elif defined(ODB_DATABASE_COMMON)
#    pragma db type("DYMMU") // Necessary to make it a value.
#  else
#    error unknown database
#  endif
#endif
  std::array<char, 17> a_;
#endif

  char c_;
  char c1_;

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_MYSQL)
#    pragma db type("BINARY(16)")
#  elif defined(ODB_DATABASE_SQLITE)
#    pragma db type("BLOB")
#  elif defined(ODB_DATABASE_PGSQL)
#    pragma db type("BYTEA")
#  elif defined(ODB_DATABASE_ORACLE)
#    pragma db type("RAW(16)")
#  elif defined(ODB_DATABASE_MSSQL)
#    pragma db type("BINARY(16)")
#  elif defined(ODB_DATABASE_COMMON)
#  else
#    error unknown database
#  endif
#endif
  char b_[16];
};

#endif // TEST_HXX
