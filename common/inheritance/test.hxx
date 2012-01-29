// file      : common/inheritance/test.hxx
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

#pragma db object abstract
struct abstract_base
{
  comp comp_;

  unsigned int num_;
  std::string str_;

  std::vector<std::string> strs_;

  bool
  operator== (const abstract_base& y) const
  {
    return
      comp_ == y.comp_ &&
      num_ == y.num_ &&
      str_ == y.str_ &&
      strs_ == y.strs_;
  }
};

#pragma db object
struct base: abstract_base
{
  #pragma db id auto
  unsigned long id_;

  bool
  operator== (const base& y) const
  {
    return id_ == y.id_ && static_cast<const abstract_base&> (*this) == y;
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

// Reference to derived object.
//
#pragma db object
struct reference
{
  #pragma db id auto
  unsigned long id_;

  object1* o1_;
};

// Multiple inheritance.
//
#pragma db object abstract
struct id_base
{
  #pragma db id auto
  unsigned long id_;

  bool
  operator== (const id_base& y) const
  {
    return id_ == y.id_;
  }
};

#pragma db object
struct object3: abstract_base, id_base
{
  bool
  operator== (const object3& y) const
  {
    return
      static_cast<const abstract_base&> (*this) == y &&
      static_cast<const id_base&> (*this) == y;
  }
};

// Empty derived object.
//
#pragma db object
struct empty: base
{
};

#endif // TEST_HXX
