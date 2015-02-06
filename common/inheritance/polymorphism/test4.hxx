// file      : common/inheritance/polymorphism/test4.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST4_HXX
#define TEST4_HXX

#include <string>

#include <odb/core.hxx>

// Test views.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object polymorphic
  struct root1
  {
    virtual ~root1 () {}
    root1 () {}
    root1 (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;
  };

  #pragma db object
  struct base1: root1
  {
    base1 () {}
    base1 (unsigned long i, unsigned long n): root1 (i), num (n) {}

    unsigned long num;
  };

  #pragma db object polymorphic
  struct root2
  {
    virtual ~root2 () {}
    root2 () {}
    root2 (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
  };

  #pragma db object
  struct base2: root2
  {
    base2 () {}
    base2 (unsigned long i, unsigned long n, const std::string& s)
        : root2 (i, n), str (s) {}

    std::string str;
  };

  // Test custom join condition.
  //
  #pragma db view object(base2) object(base1: base2::num == base1::num)
  struct view1
  {
    std::string str;
  };

  #pragma db view object(base2)
  struct view2
  {
    #pragma db column("min(" + base2::num + ")")
    unsigned long min_num;
  };

  // Test custom join condition that uses object id. It cannot come
  // from the base since the base table hasn't been join'ed yet.
  //
  #pragma db view object(base1) object(base2: base2::id == base1::id)
  struct view3
  {
    std::string str;
  };
}

#endif // TEST4_HXX
