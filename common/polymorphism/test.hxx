// file      : common/polymorphism/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <string>
#include <vector>
#include <memory>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#endif

#include <odb/core.hxx>
#include <odb/callback.hxx>
#include <odb/lazy-ptr.hxx>

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

// Test inverse object pointers in polymorhic bases.
//
#pragma db namespace table("t2_")
namespace test2
{
  struct root;

  #pragma db object
  struct root_pointer
  {
    root_pointer (root* r = 0): p (r) {}

    #pragma db id auto
    unsigned long id;

    root* p;
  };

  #pragma db object polymorphic
  struct root
  {
    virtual ~root () {}
    root () {}
    root (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(p)
    odb::lazy_ptr<root_pointer> rp;
  };

  struct base;

  #pragma db object
  struct base_pointer
  {
    base_pointer (base* b = 0) {if (b != 0) vp.push_back (b);}

    #pragma db id auto
    unsigned long id;

    std::vector<base*> vp;
  };

  #pragma db object
  struct base: root
  {
    base () {}
    base (unsigned long i, const std::string& s): root (i), str (s) {}

    std::string str;

    #pragma db inverse(vp)
    odb::lazy_ptr<base_pointer> bp;
  };

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, const std::string& s, unsigned long n)
        : base (i, s), num (n) {}

    unsigned long num;
  };

  // Views.
  //
  #pragma db view object(root_pointer = rp) object(root)
  struct root_view
  {
    #pragma db column(rp::id)
    unsigned long rp_id;

    #pragma db column(root::id)
    unsigned long r_id;
  };

  #pragma db view object(base_pointer) object(base = b)
  struct base_view
  {
    #pragma db column(base_pointer::id)
    unsigned long bp_id;

    #pragma db column(b::id)
    unsigned long b_id;

    std::string str;
  };
}

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

// Test views.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object polymorphic
  struct root1
  {
    virtual ~root1 () {}
    root1 () {}
    root1 (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;
  };

  #pragma db object
  struct base1: root1
  {
    base1 () {}
    base1 (unsigned long i, unsigned long n): root1 (i), num (n) {}

    unsigned long num;
  };

  #pragma db object polymorphic
  struct root2
  {
    virtual ~root2 () {}
    root2 () {}
    root2 (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
  };

  #pragma db object
  struct base2: root2
  {
    base2 () {}
    base2 (unsigned long i, unsigned long n, const std::string& s)
        : root2 (i, n), str (s) {}

    std::string str;
  };

  // Test custom join condition.
  //
  #pragma db view object(base2) object(base1: base2::num == base1::num)
  struct view1
  {
    std::string str;
  };

  #pragma db view object(base2)
  struct view2
  {
    #pragma db column("min(" + base2::num + ")")
    std::size_t min_num;
  };
}

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

// Test polymorphism and callbacks.
//
#pragma db namespace table("t6_")
namespace test6
{
  #pragma db object polymorphic pointer(std::auto_ptr)
  struct root
  {
    virtual ~root () {}
    root (): id (0) {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
  };

  #pragma db object callback(db_callback)
  struct base: root
  {
    base () {}
    base (unsigned long i, unsigned long n, const std::string& s)
        : root (i, n), str (s) {}

    std::string str;

    void
    db_callback (odb::callback_event, odb::database&);

    void
    db_callback (odb::callback_event, odb::database&) const;
  };

  #pragma db object callback(db_callback)
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;

    std::auto_ptr<root> ptr;

    void
    db_callback (odb::callback_event, odb::database&) const;
  };
}

// Test polymorphism and object cache (session).
//
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
#pragma db namespace table("t7_")
namespace test7
{
#ifdef HAVE_CXX11
  using std::shared_ptr;
#else
  using std::tr1::shared_ptr;
#endif

  #pragma db object polymorphic pointer(shared_ptr)
  struct root
  {
    virtual ~root () {}
    root (): id (0) {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
  };

  #pragma db object
  struct base: root
  {
    base () {}
    base (unsigned long i, unsigned long n, const std::string& s)
        : root (i, n), str (s) {}

    std::string str;
  };

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;
  };
}
#endif

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

// Test polymorphic classes with auto id.
//
#pragma db namespace table("t12_")
namespace test12
{
  #pragma db object polymorphic
  struct root
  {
    virtual ~root () = 0; // Auto-abstract.

    #pragma db id auto
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

#endif // TEST_HXX
