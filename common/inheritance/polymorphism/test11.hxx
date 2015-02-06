// file      : common/inheritance/polymorphism/test11.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST11_HXX
#define TEST11_HXX

#include <string>
#include <vector>
#include <typeinfo>

#include <odb/core.hxx>

// Test mixing reuse and polymorphic inheritance.
//
#pragma db namespace table("t11_")
namespace test11
{
  #pragma db object abstract
  struct root
  {
    root () {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
    std::vector<std::string> strs;
  };

  #pragma db object polymorphic
  struct base: root
  {
    virtual ~base () {}
    base () {}
    base (unsigned long i, unsigned long n, const std::string& s)
        : root (i, n), str (s) {}

    std::string str;
    std::vector<unsigned long> nums;

    virtual bool
    compare (const base& b, bool tc = true) const
    {
      if (tc && typeid (b) != typeid (base))
        return false;

      return id == b.id && num == b.num && strs == b.strs &&
        str == b.str && nums == b.nums;
    }
  };

  inline bool
  operator== (const base& x, const base& y) {return x.compare (y);}

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;

    virtual bool
    compare (const base& b, bool tc = true) const
    {
      if (tc && typeid (b) != typeid (derived))
        return false;

      const derived& d (static_cast<const derived&> (b));
      return base::compare (b, false) && dnum == d.dnum && dstr == d.dstr;
    }
  };
}

#endif // TEST11_HXX
