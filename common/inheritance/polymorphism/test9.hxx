// file      : common/inheritance/polymorphism/test9.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST9_HXX
#define TEST9_HXX

#include <string>
#include <vector>
#include <typeinfo>

#include <odb/core.hxx>

// Test polymorphism and readonly classes.
//
#pragma db namespace table("t9_")
namespace test9
{
  //
  // ro_root, rw_base, ro_derived
  //
  #pragma db object polymorphic readonly
  struct ro_root
  {
    virtual ~ro_root () {}
    ro_root () {}
    ro_root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
    std::vector<std::string> strs;

    virtual bool
    compare (const ro_root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (ro_root))
        return false;

      return id == r.id && num == r.num && strs == r.strs;
    }
  };

  inline bool
  operator== (const ro_root& x, const ro_root& y) {return x.compare (y);}

  #pragma db object
  struct rw_base: ro_root
  {
    rw_base () {}
    rw_base (unsigned long i, unsigned long n, const std::string& s)
        : ro_root (i, n), str (s) {}

    std::string str;
    std::vector<unsigned long> nums;

    virtual bool
    compare (const ro_root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (rw_base))
        return false;

      const rw_base& b (static_cast<const rw_base&> (r));
      return ro_root::compare (r, false) && str == b.str && nums == b.nums;
    }
  };

  #pragma db object readonly
  struct ro_derived: rw_base
  {
    ro_derived () {}
    ro_derived (unsigned long i, unsigned long n, const std::string& s)
        : rw_base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;

    virtual bool
    compare (const ro_root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (ro_derived))
        return false;

      const ro_derived& d (static_cast<const ro_derived&> (r));
      return rw_base::compare (r, false) && dnum == d.dnum && dstr == d.dstr;
    }
  };

  //
  // rw_root, ro_base, rw_derived
  //
  #pragma db object polymorphic
  struct rw_root
  {
    virtual ~rw_root () {}
    rw_root () {}
    rw_root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
    std::vector<std::string> strs;

    virtual bool
    compare (const rw_root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (rw_root))
        return false;

      return id == r.id && num == r.num && strs == r.strs;
    }
  };

  inline bool
  operator== (const rw_root& x, const rw_root& y) {return x.compare (y);}

  #pragma db object readonly
  struct ro_base: rw_root
  {
    ro_base () {}
    ro_base (unsigned long i, unsigned long n, const std::string& s)
        : rw_root (i, n), str (s) {}

    std::string str;
    std::vector<unsigned long> nums;

    virtual bool
    compare (const rw_root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (ro_base))
        return false;

      const ro_base& b (static_cast<const ro_base&> (r));
      return rw_root::compare (r, false) && str == b.str && nums == b.nums;
    }
  };

  #pragma db object
  struct rw_derived: ro_base
  {
    rw_derived () {}
    rw_derived (unsigned long i, unsigned long n, const std::string& s)
        : ro_base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;

    virtual bool
    compare (const rw_root& r, bool tc = true) const
    {
      if (tc && typeid (r) != typeid (rw_derived))
        return false;

      const rw_derived& d (static_cast<const rw_derived&> (r));
      return ro_base::compare (r, false) && dnum == d.dnum && dstr == d.dstr;
    }
  };
}

#endif // TEST9_HXX
