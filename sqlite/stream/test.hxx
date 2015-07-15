// file      : sqlite/stream/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include <odb/sqlite/text.hxx>
#include <odb/sqlite/blob.hxx>

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id;

  odb::sqlite::text t;

  #pragma db column("_123foo_bar")
  odb::sqlite::blob b;

  std::vector<odb::sqlite::blob> bv;

  odb::nullable<odb::sqlite::blob> nb;
};

#pragma db view object(object)
struct view
{
  odb::sqlite::blob b;
};

#endif // TEST_HXX
