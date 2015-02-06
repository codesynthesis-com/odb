// file      : common/inheritance/polymorphism/test3.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST3_HXX
#define TEST3_HXX

#include <string>
#include <typeinfo>

#include <odb/core.hxx>

// Test delayed loading.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () {}
    root () {}
    root (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (root))
        return false;

      return id == r.id;
    }
  };

  inline bool
  operator== (const root& x, const root& y) {return x.compare (y);}

  #pragma db object
  struct base: root
  {
    virtual ~base () {delete rptr;}
    base (): rptr (0) {}
    base (unsigned long i, unsigned long n): root (i), num (n), rptr (0) {}

    unsigned long num;
    root* rptr;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (base))
        return false;

      const base& b (static_cast<const base&> (r));
      return
        root::compare (r, false) &&
        num == b.num &&
        ((rptr == 0 && b.rptr == 0) || rptr->compare (*b.rptr));
    }
  };

  #pragma db object
  struct derived: base
  {
    virtual ~derived () {delete bptr;}
    derived (): bptr (0) {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n), str (s), bptr (0) {}

    std::string str;
    base* bptr;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (derived))
        return false;

      const derived& d (static_cast<const derived&> (r));
      return
        base::compare (r, false) &&
        str == d.str &&
        ((bptr == 0 && d.bptr == 0) || bptr->compare (*d.bptr));
    }
  };

  // Views.
  //
  #pragma db view object(base) object(root = r)
  struct base_view
  {
    #pragma db column(base::id)
    unsigned long b_id;

    #pragma db column(r::id)
    unsigned long r_id;

    unsigned long num;
  };

  #pragma db view                               \
    object(derived = d)                         \
    object(base = b)                            \
    object(root = r: d::rptr)
  struct derived_view
  {
    #pragma db column(d::id)
    unsigned long d_id;

    #pragma db column(b::id)
    unsigned long b_id;

    #pragma db column(r::id)
    unsigned long r_id;

    #pragma db column(d::num)
    unsigned long d_num;

    #pragma db column(b::num)
    unsigned long b_num;

    std::string str;
  };

  // This is an example of a pathological case, where the right-hand-side
  // of the join condition comes from one of the bases. As a result, we
  // join the base table first, which means we will get both bases and
  // derived objects instead of just derived.
  //
  //#pragma db view object(root = r) object(derived = d)
  #pragma db view object(derived = d) object(root = r)
  struct root_view
  {
    #pragma db column(r::id)
    unsigned long r_id;

    #pragma db column(d::id)
    unsigned long d_id;

    std::string str;
  };
}

#endif // TEST3_HXX
