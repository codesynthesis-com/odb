// file      : common/query/one/test.hxx
// copyright : Copyright (c) 2009-2014 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#pragma db object
struct object
{
  object (unsigned long id)
      : id_ (id)
  {
  }

  object ()
  {
  }

  #pragma db id
  unsigned long id_;
  std::string str_;
};

#endif // TEST_HXX
