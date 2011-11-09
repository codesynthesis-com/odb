// file      : common/blob/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>
#include <cstring> // std::memcmp

#include <odb/core.hxx>

#ifdef ODB_COMPILER
#  ifdef ODB_DATABASE_PGSQL
#    define BLOB_TYPE "BYTEA"
#  else
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

  // Make sure we can still use std::vector<char> as a container.
  //
  std::vector<unsigned char> cont;
};

inline bool
operator== (const object& x, const object& y)
{
  return
    x.id_ == y.id_ &&
    x.vc == y.vc &&
    x.vuc == y.vuc &&
    std::memcmp (x.c, y.c, sizeof (x.c)) == 0 &&
    std::memcmp (x.uc, y.uc, sizeof (x.uc)) == 0 &&
    x.cont == y.cont;
}

#endif // TEST_HXX
