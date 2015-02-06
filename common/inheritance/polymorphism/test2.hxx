// file      : common/inheritance/polymorphism/test2.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST2_HXX
#define TEST2_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

// Test inverse object pointers in polymorhic bases.
//
#pragma db namespace table("t2_")
namespace test2
{
  struct root;

  #pragma db object
  struct root_pointer
  {
    root_pointer (root* r = 0): p (r) {}

    #pragma db id auto
    unsigned long id;

    root* p;
  };

  #pragma db object polymorphic
  struct root
  {
    virtual ~root () {}
    root () {}
    root (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(p)
    odb::lazy_ptr<root_pointer> rp;
  };

  struct base;

  #pragma db object
  struct base_pointer
  {
    base_pointer (base* b = 0) {if (b != 0) vp.push_back (b);}

    #pragma db id auto
    unsigned long id;

    std::vector<base*> vp;
  };

  #pragma db object
  struct base: root
  {
    base () {}
    base (unsigned long i, const std::string& s): root (i), str (s) {}

    std::string str;

    #pragma db inverse(vp)
    odb::lazy_ptr<base_pointer> bp;
  };

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, const std::string& s, unsigned long n)
        : base (i, s), num (n) {}

    unsigned long num;
  };

  // Views.
  //
  #pragma db view object(root_pointer = rp) object(root)
  struct root_view
  {
    #pragma db column(rp::id)
    unsigned long rp_id;

    #pragma db column(root::id)
    unsigned long r_id;
  };

  #pragma db view object(base_pointer) object(base = b)
  struct base_view
  {
    #pragma db column(base_pointer::id)
    unsigned long bp_id;

    #pragma db column(b::id)
    unsigned long b_id;

    std::string str;
  };
}

#endif // TEST2_HXX
