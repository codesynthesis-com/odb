// file      : common/inheritance/polymorphism/test13.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST13_HXX
#define TEST13_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

// Test polymorphic derived without any non-container data members (which
// results in an empty SELECT statement).
//
#pragma db namespace table("t13_")
namespace test13
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;
  };

  #pragma db object
  struct base: root
  {
    std::vector<int> nums;
  };

  #pragma db object
  struct derived: base
  {
    std::vector<std::string> strs;
  };

  #pragma db object
  struct base1: root
  {
    // Nothing.
  };
}

#endif // TEST13_HXX
