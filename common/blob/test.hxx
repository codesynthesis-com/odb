// file      : common/blob/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <vector>
#include <cstring> // std::memcmp

#ifdef HAVE_CXX11
#  include <array>
#endif

#include <odb/core.hxx>

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_PGSQL)
#    define BLOB_TYPE "BYTEA"
#  elif defined(ODB_DATABASE_MSSQL)
//#    define BLOB_TYPE "VARBINARY(1024)"
#    define BLOB_TYPE "VARBINARY(max)"
#  else
//#    define BLOB_TYPE "RAW(1024)"
#    define BLOB_TYPE "BLOB"
#  endif
#endif

#pragma db object
struct object
{
  object () {}
  object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  #pragma db type(BLOB_TYPE)
  std::vector<char> vc;

  #pragma db type(BLOB_TYPE)
  std::vector<unsigned char> vuc;

  #pragma db type(BLOB_TYPE)
  char c[1024];

  #pragma db type(BLOB_TYPE)
  unsigned char uc[1024];

#ifdef HAVE_CXX11
  #pragma db type(BLOB_TYPE)
  std::array<char, 1024> a;

  #pragma db type(BLOB_TYPE)
  std::array<char, 1024> ua;
#endif

  // Make sure we can still use std::vector<char> and std::array<char>
  // as containers.
  //
  std::vector<unsigned char> cont;
};

inline bool
operator== (const object& x, const object& y)
{
  return x.id_ == y.id_
    && x.vc == y.vc
    && x.vuc == y.vuc
    && std::memcmp (x.c, y.c, sizeof (x.c)) == 0
    && std::memcmp (x.uc, y.uc, sizeof (x.uc)) == 0
#ifdef HAVE_CXX11
    && x.a == y.a
    && x.ua == y.ua
#endif
    && x.cont == y.cont;
}

#endif // TEST_HXX
