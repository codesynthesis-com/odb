// file      : common/view/olv/test5.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST5_HXX
#define TEST5_HXX

#include <string>
#include <memory>  // shared_ptr
#include <utility> // pair

#include <odb/core.hxx>

// Test NULL object pointers.
//
#pragma db namespace table("t5_") pointer(std::shared_ptr) session
namespace test5
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
    shared_ptr<object1> o1;
    shared_ptr<object2> o2;
  };

  typedef std::pair<int, int> comp_id;
  #pragma db value(comp_id)

  #pragma db object
  struct object3
  {
    object3 (comp_id id_ = comp_id (), int n_ = 0): id (id_), n (n_) {}

    #pragma db id
    comp_id id;

    int n;
  };

  #pragma db object
  struct object4
  {
    object4 () {}
    object4 (const char* s_, shared_ptr<object3> o3_): s (s_), o3 (o3_) {}

    #pragma db id auto
    int id;

    std::string s;
    shared_ptr<object3> o3;
  };

  #pragma db view object(object3) object(object4)
  struct view2
  {
    shared_ptr<object4> o4;
    shared_ptr<object3> o3;
  };
}

#endif // TEST5_HXX
