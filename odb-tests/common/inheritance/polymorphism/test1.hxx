// file      : common/inheritance/polymorphism/test1.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST1_HXX
#define TEST1_HXX

#include <string>
#include <vector>
#include <typeinfo>

#include <odb/core.hxx>

// Test basic polymorphism functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () {}
    root () {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id column("object_id")
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

  // Views.
  //
  #pragma db view object(root)
  struct root_view
  {
    //#pragma db column(root::typeid_)
    std::string typeid_; // @@ tmp

    unsigned long num;
  };

  #pragma db view object(base = b)
  struct base_view
  {
    unsigned long id;
    unsigned long num;
    std::string str;
  };

  #pragma db view object(derived)
  struct derived_view
  {
    unsigned long num;
    std::string str;
    unsigned long dnum;
    std::string dstr;
  };
}

#endif // TEST1_HXX
