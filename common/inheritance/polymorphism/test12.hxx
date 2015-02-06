// file      : common/inheritance/polymorphism/test12.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST12_HXX
#define TEST12_HXX

#include <typeinfo>

#include <odb/core.hxx>

// Test polymorphic classes with private auto id.
//
#pragma db namespace table("t12_")
namespace test12
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () = 0; // Auto-abstract.

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (root))
        return false;

      return id_ == r.id_;
    }

    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id auto access(id)
    unsigned long id_;
  };

  inline root::
  ~root () {}

  inline bool
  operator== (const root& x, const root& y) {return x.compare (y);}

  #pragma db object
  struct base: root
  {
    base () {}
    base (unsigned long n): num (n) {}

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
    derived (unsigned long n): base (n) {}

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (derived))
        return false;

      return base::compare (r, false);
    }
  };
}

#endif // TEST12_HXX
