// file      : common/view/olv/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <memory>  // unique_ptr
#include <utility> // pair

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>
#include <odb/section.hxx>

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
    unique_ptr<object2> o2;
    unique_ptr<object1> o1;
  };
}

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
    unique_ptr<object2> o2;
    unique_ptr<object1> o1;
  };
}

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

#endif // TEST_HXX
