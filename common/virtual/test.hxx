// file      : common/virtual/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

// Test basic virtual data member functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db value
  struct comp
  {
    int i;

    #pragma db transient
    std::string s;
    #pragma db member(s_) virtual(std::string) access(s)

    bool operator== (const comp& v) const
    {
      return i == v.i && s == v.s;
    }
  };

  #pragma db object transient
  struct object
  {
    object (): p1 (0) {}
    ~object () {delete p1;}

    struct {unsigned long v;} id1;

    #pragma db id
    #pragma db member(id) get(id1.v) virtual(unsigned long) set(id1.v)
    #pragma db member(id) auto

    int i () const {return i1;}
    void i (int i) {i1 = i;}
    int i1;

    comp c1;
    #pragma db member(c) virtual(comp) access(c1)

    typedef std::vector<std::string> strings;
    strings v1;
    #pragma db member(v) virtual(strings) access(v1)

    typedef object* object_ptr;
    object_ptr p1;
    #pragma db member(p) virtual(object_ptr) access(p1)

    bool operator== (const object& o) const
    {
      return id1.v == o.id1.v &&
        i1 == o.i1 &&
        c1 == o.c1 &&
        v1 == o.v1 &&
        (p1 != 0 ? o.p1 != 0 && *p1 == *o.p1 : o.p1 == 0);
    }
  };

  #pragma db member(object::i) virtual(int)
}

#pragma db member(test1::object::id) column("oid")

// Test pragma resolution to virtual data member.
//
#pragma db namespace table("t2_")
namespace test2
{
  struct object1;
  struct object2;

  typedef object1* object1_ptr;
  typedef object2* object2_ptr;

  #pragma db object
  struct object2
  {
    object2 (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(o)
    object1_ptr o1;
  };

  #pragma db object
  struct object1
  {
    object1 (unsigned long i = 0): id (i), o2 (0) {}
    ~object1 () {delete o2;}

    #pragma db id
    unsigned long id;

    #pragma db transient
    object2_ptr o2;
    #pragma db member(o) virtual(object2_ptr) access(o2)

    #pragma db transient
    unsigned long n1;
    #pragma db member(n) virtual(unsigned long) access(n1)
    #pragma db index member(n)
  };

  #pragma db view object(object1) object(object2)
  struct view1
  {
    #pragma db column(object1::n)
    unsigned long i;
  };

  #pragma db view object(object1 = o1) object(object2 = o2: o1::n == o2::id)
  struct view2
  {
    #pragma db column(o1::n)
    unsigned long i;
  };

  #pragma db view object(object1: object1::n != 0)
  struct view3
  {
    #pragma db column(test2::object1::n)
    unsigned long i;
  };
}

// Use virtual data members to implement multi-member composite object id.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db value
  struct name
  {
    name () {}
    name (const std::string& f, const std::string& l)
        : first (f), last(l) {}

    std::string first;
    std::string last;

    bool operator< (const name& x) const
    {
      return first < x.first || (first == x.first && last < x.last);
    }
  };

  #pragma db object transient
  struct person
  {
    std::string first_;
    std::string last_;

    #pragma db member(name) virtual(name) id                       \
               get(::test3::name (this.first_, this.last_))        \
               set(this.first_ = (?).first; this.last_ = (?).last)
  };
}

#endif // TEST_HXX
