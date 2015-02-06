// file      : common/inheritance/polymorphism/test5.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST5_HXX
#define TEST5_HXX

#include <string>
#include <vector>
#include <memory>
#include <typeinfo>

#include <odb/core.hxx>

// Test polymorphism and optimistic concurrency.
//
#pragma db namespace table("t5_")
namespace test5
{
  #pragma db object polymorphic optimistic pointer(std::auto_ptr)
  struct root
  {
    virtual ~root () {}
    root () {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    #pragma db version
    unsigned long version;

    unsigned long num;
    std::vector<std::string> strs;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (root))
        return false;

      return id == r.id && version == r.version &&
        num == r.num && strs == r.strs;
    }
  };

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

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;

    virtual bool
    compare (const root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (derived))
        return false;

      const derived& d (static_cast<const derived&> (r));
      return base::compare (r, false) && dnum == d.dnum && dstr == d.dstr;
    }
  };
}

#endif // TEST5_HXX
