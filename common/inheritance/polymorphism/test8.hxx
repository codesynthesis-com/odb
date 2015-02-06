// file      : common/inheritance/polymorphism/test8.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST8_HXX
#define TEST8_HXX

#include <string>
#include <vector>
#include <typeinfo>

#include <odb/core.hxx>

// Test polymorphism and abstract bases.
//
#pragma db namespace table("t8_")
namespace test8
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () = 0; // Auto-abstract.
    root () {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
    std::vector<std::string> strs;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (root))
        return false;

      return id == r.id && num == r.num && strs == r.strs;
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
    base (unsigned long i, unsigned long n, const std::string& s)
        : root (i, n), str (s) {}

    std::string str;
    std::vector<unsigned long> nums;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (base))
        return false;

      const base& b (static_cast<const base&> (r));
      return root::compare (r, false) && str == b.str && nums == b.nums;
    }
  };

  #pragma db object abstract
  struct interm: base
  {
    interm () {}
    interm (unsigned long i, unsigned long n, const std::string& s, bool b)
        : base (i, n, s), bln (b) {}

    bool bln;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (interm))
        return false;

      const interm& i (static_cast<const interm&> (r));
      return base::compare (r, false) && bln == i.bln;
    }
  };

  #pragma db object
  struct derived1: interm
  {
    derived1 () {}
    derived1 (unsigned long i, unsigned long n, const std::string& s, bool b)
        : interm (i, n, s, b), dnum (n + 1) {}

    unsigned long dnum;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (derived1))
        return false;

      const derived1& d (static_cast<const derived1&> (r));
      return interm::compare (r, false) && dnum == d.dnum;
    }
  };

  #pragma db object
  struct derived2: interm
  {
    derived2 () {}
    derived2 (unsigned long i, unsigned long n, const std::string& s, bool b)
        : interm (i, n, s, b), dstr (s + 'd') {}

    std::string dstr;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (derived2))
        return false;

      const derived2& d (static_cast<const derived2&> (r));
      return interm::compare (r, false) && dstr == d.dstr;
    }
  };
}

#endif // TEST8_HXX
