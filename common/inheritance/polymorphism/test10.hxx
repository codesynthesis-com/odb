// file      : common/inheritance/polymorphism/test10.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST10_HXX
#define TEST10_HXX

#include <typeinfo>

#include <odb/core.hxx>

// Test empty polymorphic classes.
//
#pragma db namespace table("t10_")
namespace test10
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () = 0; // Auto-abstract.
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

  inline root::
  ~root () {}

  inline bool
  operator== (const root& x, const root& y) {return x.compare (y);}

  #pragma db object
  struct base: root
  {
    base () {}
    base (unsigned long i, unsigned long n): root (i), num (n) {}

    unsigned long num;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (base))
        return false;

      const base& b (static_cast<const base&> (r));
      return root::compare (r, false) && num == b.num;
    }
  };

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n): base (i, n) {}

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (derived))
        return false;

      return base::compare (r, false);
    }
  };
}

#endif // TEST10_HXX
