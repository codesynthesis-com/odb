// file      : common/view/olv/test3.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST3_HXX
#define TEST3_HXX

#include <string>
#include <vector>
#include <memory>  // shared_ptr

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

// Test JOINs for pointed-to objects, existing and automatically added.
//
#pragma db namespace table("t3_") pointer(std::shared_ptr) session
namespace test3
{
  using std::shared_ptr;

  struct object2;

  #pragma db object
  struct object1
  {
    object1 (int n_ = 0): n (n_) {}

    #pragma db id auto
    int id;

    int n;

    #pragma db inverse(o1)
    odb::lazy_weak_ptr<object2> o2;
  };

  #pragma db object
  struct object2
  {
    object2 (const char* s_ = ""): s (s_) {}

    #pragma db id auto
    int id;

    std::string s;

    shared_ptr<object1> o1;
  };

  #pragma db view object(object1) object(object2)
  struct view1a // Existing JOIN.
  {
    shared_ptr<object1> o1;
  };

  #pragma db view object(object1)
  struct view1b // Automatic JOIN.
  {
    shared_ptr<object1> o1;
  };

  // Container case.
  //
  struct object4;

  #pragma db object
  struct object3
  {
    object3 (int n_ = 0): n (n_) {}

    #pragma db id auto
    int id;

    int n;

    #pragma db inverse(o3)
    odb::lazy_weak_ptr<object4> o4;
  };

  #pragma db object
  struct object4
  {
    object4 (const char* s_ = ""): s (s_) {}

    #pragma db id auto
    int id;

    std::string s;

    std::vector<shared_ptr<object3>> o3;
  };

  #pragma db view object(object3) object(object4 = o4)
  struct view2a // Existing JOIN.
  {
    shared_ptr<object3> o3;
  };

  #pragma db view object(object3)
  struct view2b // Automatic JOIN.
  {
    shared_ptr<object3> o3;
  };
}

#endif // TEST3_HXX
