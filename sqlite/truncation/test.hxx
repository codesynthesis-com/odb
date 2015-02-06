// file      : sqlite/truncation/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <odb/core.hxx>

#pragma db object table ("test")
struct object1
{
  object1 (unsigned long id)
      : id_ (id)
  {
  }

  object1 ()
  {
  }

  #pragma db id
  unsigned long id_;

  std::string str_;
};

#pragma db object table ("test")
struct object2
{
  object2 (unsigned long id)
      : id_ (id)
  {
  }

  object2 ()
  {
  }

  #pragma db id
  unsigned long id_;

  std::string str_;
};

#endif // TEST_HXX
