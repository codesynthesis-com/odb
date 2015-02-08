// file      : common/view/olv/test7.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST7_HXX
#define TEST7_HXX

#include <string>
#include <memory>  // unique_ptr
#include <utility> // std::move

#include <odb/core.hxx>

// Test explicit conversion to smart pointer member.
//
#pragma db namespace table("t7_") pointer(*) session
namespace test7
{
  using std::unique_ptr;

  #pragma db object
  struct object1
  {
    object1 (int n_ = 0): n (n_) {}

    #pragma db id auto
    int id;

    int n;
  };

  #pragma db object
  struct object2
  {
    object2 () {}
    object2 (const char* s_, object1* o1_): s (s_), o1 (o1_) {}

    #pragma db id auto
    int id;

    std::string s;
    object1* o1; // Shallow.
  };

  #pragma db view object(object1) object(object2)
  struct view1
  {
    // VC12 workaround (no default move constructor generation).
    //
    view1 () {}
    view1 (view1&& x): o2 (std::move (x.o2)), o1 (std::move (x.o1)) {}

    unique_ptr<object2> o2;
    unique_ptr<object1> o1;
  };
}

#endif // TEST7_HXX
