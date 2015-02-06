// file      : common/inheritance/transient/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

struct object;

struct base
{
  int n;
  std::vector<std::string> v;
  object* p;
};

#pragma db value
struct comp: base
{
  unsigned int num;
  std::string str;
  std::vector<unsigned int> nums;

  bool
  operator== (const comp& y) const
  {
    return num == y.num && str == y.str && nums == y.nums;
  }
};

#pragma db object
struct object: base
{
  #pragma db id auto
  unsigned int id_;

  unsigned int num;
  std::string str;
  std::vector<std::string> strs;
  comp c;

  bool
  operator== (const object& y) const
  {
    return num == y.num && str == y.str && strs == y.strs && c == y.c;
  }
};

#pragma db view object(object)
struct view: base
{
  unsigned int num;
  std::string str;
};

#endif // TEST_HXX
