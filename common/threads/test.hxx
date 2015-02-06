// file      : common/template/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object
struct object
{
  object (unsigned long id, const std::string& str)
      : id_ (id), str_ (str)
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
