// file      : common/template/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <odb/core.hxx>

#pragma odb object
struct object1
{
  object1 (unsigned long id)
      : id_ (id)
  {
  }

  object1 ()
  {
  }

  #pragma odb id
  unsigned long id_;
};

#pragma odb object
struct object2
{
  object2 (const std::string& id)
      : id_ (id)
  {
  }

  object2 ()
  {
  }

  #pragma odb id
  std::string id_;
};

#endif // TEST_HXX
