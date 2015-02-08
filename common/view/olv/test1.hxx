// file      : common/view/olv/test1.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST1_HXX
#define TEST1_HXX

#include <string>
#include <memory>  // unique_ptr

#include <odb/core.hxx>

// Test basic object loading functionality.
//
#pragma db namespace table("t1_") pointer(std::unique_ptr)
namespace test1
{
  #pragma db object
  struct object1
  {
    object1 (int id_ = 0, int n_ = 0): id (id_), n (n_) {}

    #pragma db id
    int id;

    int n;
  };

  #pragma db object
  struct object2
  {
    object2 (int id_ = 0, const char* s_ = ""): id (id_), s (s_) {}

    #pragma db id
    int id;

    std::string s;
  };

  #pragma db view object(object1) object(object2: object1::id == object2::id)
  struct view1
  {
    std::unique_ptr<object2> o2;
  };

  #pragma db view object(object1) object(object2: object1::id == object2::id)
  struct view2
  {
    std::unique_ptr<object2> o2;
    std::unique_ptr<object1> o1;
  };

  #pragma db view object(object1 = o1) object(object2 = o2: o1::id == o2::id)
  struct view3
  {
    std::unique_ptr<object1> o1;
    std::unique_ptr<object2> o2;
  };

  #pragma db view object(object1 = o1) object(object2 = o2: o1::id == o2::id)
  struct view4
  {
    std::string s;
    std::unique_ptr<object2> o2;

    #pragma db column(o1::id)
    int id;

    std::unique_ptr<object1> o1;
    int n;
  };

  #pragma db view                                         \
    object(object1)                                       \
    object(object2: object1::id == object2::id)           \
    object(object1 = o1b: object1::id == o1b::n)
  struct view5
  {
    std::unique_ptr<object1> o1a;
    std::unique_ptr<object2> o2;
    std::unique_ptr<object1> o1b;
  };
}

#endif // TEST1_HXX
