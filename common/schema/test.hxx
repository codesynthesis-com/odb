// file      : common/schema/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <odb/core.hxx>

// Table names.
//
#pragma odb object table ("TABLE_EXPLICIT")
struct table_explicit
{
  #pragma odb id
  unsigned long id_;
};

#pragma odb object
struct table_implicit
{
  #pragma odb id
  unsigned long id_;
};

// Column names.
//
#pragma odb object
struct column
{
  #pragma odb id
  int m1;

  #pragma odb column ("foo")
  int m2;

  int m_m3;
  int _m4;
  int m5_;
  int m_;
  int m__;
};

// Column types.
//
#pragma odb object
struct type
{
  #pragma odb id
  int id;

  #pragma odb type ("INTEGER")
  bool m1;
};

#endif // TEST_HXX
