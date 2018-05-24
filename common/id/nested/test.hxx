// file      : common/id/nested/test.hxx
// copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/vector.hxx>

// Simple nested id.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db value
  struct comp
  {
    int x;
    std::string y;
  };

  #pragma db object
  struct object
  {
    #pragma db id(y)
    comp id;

    int z;
    std::vector<int> v;

    object () {}
    object (int x, std::string y, int z_): z (z_) {id.x = x; id.y = y;}
  };

  inline bool
  operator== (object a, object b)
  {
    return a.id.x == b.id.x && a.id.y == b.id.y && a.z == b.z && a.v == b.v;
  }

  #pragma db object
  struct object1
  {
    #pragma db id auto
    int id;

    object* p;

    object1 (object* p_ = 0): p (p_) {}
    ~object1 () {delete p;}
  };

  inline bool
  operator== (const object1& a, const object1& b)
  {
    return a.id == b.id && *a.p == *b.p;
  }
}

// Composite nested id.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db value
  struct comp1
  {
    int x;
    int y;
  };

  #pragma db value
  struct comp2
  {
    comp1 c;
    std::string s;
  };

  #pragma db object
  struct object
  {
    #pragma db id(c)
    comp2 id;

    int z;
    odb::vector<int> v;

    object () {}
    object (int x, int y, std::string s, int z_)
        : z (z_) {id.c.x = x; id.c.y = y; id.s = s;}
  };

  inline bool
  operator== (object a, object b)
  {
    return a.id.c.x == b.id.c.x && a.id.c.y == b.id.c.y &&
      a.id.s == b.id.s && a.z == b.z && a.v == b.v;
  }

  #pragma db object
  struct object1
  {
    #pragma db id auto
    int id;

    object* p;

    object1 (object* p_ = 0): p (p_) {}
    ~object1 () {delete p;}
  };

  inline bool
  operator== (const object1& a, const object1& b)
  {
    return a.id == b.id && *a.p == *b.p;
  }

  // Multiple levels of nesting, just a compile test.
  //
  #pragma db object
  struct object2
  {
    #pragma db id(c.x)
    comp2 id;

    int z;
  };
}

// Custom/by-value access.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db value
  struct comp
  {
    int x;

    std::string get_y () const {return y;}
    void set_y (std::string v) {y = v;}

    #pragma db get(get_y) set(set_y)
    std::string y;
  };

  #pragma db object
  struct object
  {
    comp get_id () const {return id;}
    void set_id (comp v) {id = v;}

    #pragma db id(y) get(get_id) set(set_id)
    comp id;

    int z;

    object () {}
    object (int x, std::string y, int z_): z (z_) {id.x = x; id.y = y;}
  };

  inline bool
  operator== (object a, object b)
  {
    return a.id.x == b.id.x && a.id.y == b.id.y && a.z == b.z;
  }
}

// Polymorphic.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db value
  struct comp
  {
    int x;
    std::string y;
  };

  #pragma db object polymorphic
  struct base
  {
    #pragma db id(y)
    comp id;

    virtual ~base () {}
    base () {}
    base (int x, std::string y) {id.x = x; id.y = y;}
  };

  inline bool
  operator== (const base& a, const base& b)
  {
    return a.id.x == b.id.x && a.id.y == b.id.y;
  }

  #pragma db object
  struct object: base
  {
    int z;

    object () {}
    object (int x, std::string y, int z_): base (x, y), z (z_) {}
  };

  inline bool
  operator== (const object& a, const object& b)
  {
    return a.id.x == b.id.x && a.id.y == b.id.y && a.z == b.z;
  }
}

#endif // TEST_HXX
