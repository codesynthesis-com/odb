// file      : common/erase-query/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>

struct object2;

#pragma db object
struct object
{
  object (unsigned long id)
      : id_ (id), o1 (0), o2 (0)
  {
  }

  object ()
      : o1 (0), o2 (0)
  {
  }

  #pragma db id
  unsigned long id_;

  std::vector<int> v;

  int num;

  object* o1;
  object2* o2;
};

#pragma db object
struct object2
{
  #pragma db id auto
  unsigned long id_;

  int num;
};

#endif // TEST_HXX
