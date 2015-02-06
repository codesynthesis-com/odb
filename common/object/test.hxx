// file      : common/object/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <utility> // std::pair

#include <odb/core.hxx>

// Test persistent class template instantiation.
//
#pragma db namespace table("t1_")
namespace test1
{
  typedef std::pair<unsigned long, std::string> pair_object;
  #pragma db object(pair_object)
  #pragma db member(pair_object::first) id auto

  #pragma db object abstract
  struct base_data
  {
    #pragma db id auto
    unsigned long id;
  };

  template <typename T>
  struct base: base_data
  {
    T x;
  };

  typedef base<std::string> base_derived;
  #pragma db object(base_derived) abstract

  #pragma db object
  struct derived: base_derived
  {
    int n;
  };

  // Test instantiation in order to "see" id, etc.
  //
  typedef base<int> int_base;
  #pragma db object(int_base)
}

#endif // TEST_HXX
