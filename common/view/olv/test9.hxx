// file      : common/view/olv/test9.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST9_HXX
#define TEST9_HXX

#include <string>
#include <memory>  // shared_ptr

#include <odb/core.hxx>

// Test loading polymorphic objects.
//
#pragma db namespace table("t9_") session
namespace test9
{
  using std::shared_ptr;

  #pragma db object polymorphic pointer(shared_ptr)
  struct root
  {
    virtual ~root () {}
    root (int n_ = 0): n (n_) {}

    #pragma db id auto
    int id;

    int n;
  };

  #pragma db object
  struct base: root
  {
    base (int n_ = 0, const char* s_ = ""): root (n_), s (s_) {}

    std::string s;
  };

  #pragma db object
  struct derived: base
  {
    derived (int n_ = 0, const char* s_ = "", bool b_ = false)
        : base (n_, s_), b (b_) {}

    bool b;
  };

  // Load via root.
  //
  #pragma db view object(root)
  struct view1r
  {
    shared_ptr<root> o;
    int n;
  };

  // Load via base.
  //
  #pragma db view object(base)
  struct view1b
  {
    std::string s;
    shared_ptr<base> o;
    int n;
  };

  // Load via derived.
  //
  #pragma db view object(derived)
  struct view1d
  {
    std::string s;
    shared_ptr<derived> o;
    int n;
  };
}

#endif // TEST9_HXX
