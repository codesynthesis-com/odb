// file      : common/schema/embedded/basics/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

#pragma db object
struct object
{
  #pragma db auto id
  unsigned long id;

  std::string str;

  std::vector<int> nums;
};

#endif // TEST_HXX
