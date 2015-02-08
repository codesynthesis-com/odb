// file      : common/view/olv/test2.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST2_HXX
#define TEST2_HXX

#include <string>
#include <vector>
#include <memory>  // shared_ptr

#include <odb/core.hxx>

// Test loading of object pointers inside objects.
//
#pragma db namespace table("t2_") pointer(std::shared_ptr) session
namespace test2
{
  using std::shared_ptr;

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
    object2 (const char* s_, shared_ptr<object1> o1_): s (s_), o1 (o1_) {}

    #pragma db id auto
    int id;

    std::string s;
    shared_ptr<object1> o1;
  };

  #pragma db view object(object1) object(object2)
  struct view1
  {
    shared_ptr<object2> o2;
  };

  #pragma db view object(object1) object(object2)
  struct view2
  {
    shared_ptr<object2> o2; // "Unfortunate" order.
    shared_ptr<object1> o1;
  };

  #pragma db object
  struct object3
  {
    object3 () {}
    object3 (shared_ptr<object2> o2_): o2 (o2_) {}

    #pragma db id auto
    int id;

    shared_ptr<object2> o2;
  };

  #pragma db view object(object1) object(object2) object(object3)
  struct view3
  {
    shared_ptr<object3> o3; // "Unfortunate" order.
    shared_ptr<object1> o1;
  };

  #pragma db object
  struct object4
  {
    #pragma db id auto
    int id;

    std::vector<shared_ptr<object2>> o2;
  };

  #pragma db view object(object4)
  struct view4
  {
    shared_ptr<object4> o4;
  };

  #pragma db view object(object4) object (object2) object(object1)
  struct view5
  {
    shared_ptr<object4> o4; // "Unfortunate" order.
    shared_ptr<object1> o1;
  };

  #pragma db object
  struct object5
  {
    object5 () {}
    object5 (shared_ptr<object1> o1_, shared_ptr<object2> o2_)
        : o1 (o1_), o2 (o2_) {}

    #pragma db id auto
    int id;

    shared_ptr<object1> o1;
    shared_ptr<object2> o2;
  };

  #pragma db view object(object5) object (object2)        \
    object(object1 = o1a: object2::o1)                    \
    object(object1 = o1b: object5::o1)
  struct view6
  {
    shared_ptr<object1> o1a;
    shared_ptr<object1> o1b;
  };
}

#endif // TEST2_HXX
