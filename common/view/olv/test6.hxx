// file      : common/view/olv/test6.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST6_HXX
#define TEST6_HXX

#include <string>
#include <memory>  // shared_ptr

#include <odb/core.hxx>
#include <odb/section.hxx>

// Test interaction with sections.
//
#pragma db namespace table("t6_") pointer(std::shared_ptr)
namespace test6
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

    #pragma db load(lazy)
    odb::section r;

    #pragma db section(r)
    shared_ptr<object1> o1;
  };

  #pragma db view object(object1) object(object2)
  struct view1
  {
    shared_ptr<object1> o1;
    shared_ptr<object2> o2;
  };
}

#endif // TEST6_HXX
