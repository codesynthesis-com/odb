// file      : common/view/olv/test8.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST8_HXX
#define TEST8_HXX

#include <string>
#include <memory>  // unique_ptr
#include <utility> // std::move

#include <odb/core.hxx>

// Test loading objects without id.
//
#pragma db namespace table("t8_") pointer(*) session
namespace test8
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

  #pragma db object no_id
  struct object2
  {
    object2 () {}
    object2 (const char* s_, object1* o1_): s (s_), o1 (o1_) {}

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

#endif // TEST8_HXX
