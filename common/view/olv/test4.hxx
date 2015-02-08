// file      : common/view/olv/test4.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST4_HXX
#define TEST4_HXX

#include <string>
#include <memory>  // shared_ptr

#include <odb/core.hxx>

// Test by-value load.
//
#pragma db namespace table("t4_") session
namespace test4
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
    object2 (int id_ = 0, const char* s_ = "", object1* o1_ = 0)
        : id (id_), s (s_), o1 (o1_) {}

    #pragma db id
    int id;

    std::string s;
    object1* o1; // Shallow copy.
  };

  typedef object1* object1_ptr;
  typedef object2* object2_ptr;

  #pragma db view object(object1)
  struct view1
  {
    #pragma db member(o1_) virtual(object1_ptr) get(&this.o1) set()

    #pragma db transient
    object1 o1;
  };

  #pragma db view object(object1) transient
  struct view1a
  {
    view1a (): o1_null (true) {}

    #pragma db member(o1_) virtual(object1_ptr) get(&this.o1)       \
      set(this.o1_null = !(?))

    object1 o1;
    bool o1_null;
  };

  #pragma db view object(object1)
  struct view1b
  {
    view1b (): o1_p (0) {}

    #pragma db transient
    object1 o1;

    #pragma db get(&this.o1) set(o1_p = (?))
    object1* o1_p;
  };

  #pragma db view object(object1)
  struct view1c
  {
    view1c (object1& o1): o1_p (&o1) {}

    object1* o1_p;
  };

  #pragma db view object(object1) object(object2) transient
  struct view2
  {
    #pragma db member(o2_) virtual(object2_ptr) get(&this.o2) set()
    #pragma db member(o1_) virtual(object1_ptr) get(&this.o1) set()

    object1 o1;
    object2 o2;
  };

  #pragma db view object(object1) object(object2) transient
  struct view2a
  {
    #pragma db member(o2_) virtual(object2_ptr) get(&this.o2)       \
      set(o2_null = !(?))
    #pragma db member(o1_) virtual(object1_ptr) get(&this.o1) set()

    object1 o1;
    object2 o2;
    bool o2_null;
  };

  // Test loading into raw pointer with non-raw object pointer.
  //
  using std::shared_ptr;

  #pragma db object pointer(shared_ptr)
  struct object3
  {
    object3 (int id_ = 0, int n_ = 0): id (id_), n (n_) {}

    #pragma db id
    int id;

    int n;
  };

  #pragma db view object(object3)
  struct view3
  {
    // This view implements the following slightly twisted logic: if the
    // object is already in the cache, then set o3_p to that. Otherwise,
    // load it into the by-value instance. We can also check whether o3_p
    // points to o3 to distinguish between the two outcomes.
    //

    // Since we may be getting the pointer as both smart and raw, we
    // need to create a bit of support code to use in the modifier
    // expression.
    //
    void set_o3 (object3* p) {o3_p = p;}                   // &o3 or NULL.
    void set_o3 (shared_ptr<object3> p) {o3_p = p.get ();} // From cache.

    #pragma db get(&this.o3) set(set_o3(?))
    object3* o3_p;

    #pragma db transient
    object3 o3;

    // Return-by-value support (query_value()).
    //
    view3 (): o3_p (0) {}
    view3 (const view3& x): o3_p (x.o3_p == &x.o3 ? &o3 : x.o3_p), o3 (x.o3) {}
  };
}

#endif // TEST4_HXX
