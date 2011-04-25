// file      : common/inheritance/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

#pragma db value
struct comp_base
{
  std::vector<unsigned char> bools;

  bool
  operator== (const comp_base& y) const
  {
    return bools == y.bools;
  }
};

#pragma db value
struct comp: comp_base
{
  unsigned int num;
  std::string str;

  std::vector<unsigned int> nums;

  bool
  operator== (const comp& y) const
  {
    return
      static_cast<const comp_base&> (*this) == y &&
      num == y.num &&
      str == y.str &&
      nums == y.nums;
  }
};

#pragma db object
struct base
{
  #pragma db id auto
  unsigned long id_;

  comp comp_;

  unsigned int num_;
  std::string str_;

  std::vector<std::string> strs_;

  bool
  operator== (const base& y) const
  {
    return
      id_ == y.id_ &&
      comp_ == y.comp_ &&
      num_ == y.num_ &&
      str_ == y.str_ &&
      strs_ == y.strs_;
  }
};

#pragma db object
struct object1: base
{
  unsigned int num1_;

  bool
  operator== (const object1& y) const
  {
    return static_cast<const base&> (*this) == y && num1_ == y.num1_;
  }
};

#pragma db object
struct object2: base
{
  #pragma db column ("derived_str")
  std::string str_;

  bool
  operator== (const object2& y) const
  {
    return static_cast<const base&> (*this) == y && str_ == y.str_;
  }
};

#pragma db object
struct reference
{
  #pragma db id auto
  unsigned long id_;

  object1* o1_;
};

#endif // TEST_HXX
