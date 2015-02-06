// file      : common/section/basics/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/section.hxx>
#include <odb/vector.hxx>

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_PGSQL)
#    define BLOB_TYPE "BYTEA"
#  elif defined(ODB_DATABASE_MSSQL)
#    define BLOB_TYPE "VARBINARY(max)"
#  else
#    define BLOB_TYPE "BLOB"
#  endif
#endif

// Test lazy-loaded, always-updated section.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s)
    int sn;

    int n;

    #pragma db section(s)
    std::string ss;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test lazy-loaded, change-updated section.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy) update(change)
    odb::section s;

    #pragma db section(s)
    int sn;

    int n;

    #pragma db section(s)
    std::string ss;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test lazy-loaded, manually-updated section.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy) update(manual)
    odb::section s;

    #pragma db section(s)
    int sn;

    int n;

    #pragma db section(s)
    std::string ss;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test eager-loaded, change-updated section.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    #pragma db section(s)
    int sn;

    #pragma db update(change)
    odb::section s;

    int n;

    #pragma db section(s)
    std::string ss;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test eager-loaded, manually-updated section.
//
#pragma db namespace table("t5_")
namespace test5
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    #pragma db section(s)
    int sn;

    #pragma db update(manual)
    odb::section s;

    int n;

    #pragma db section(s)
    std::string ss;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test value-only and container-only section. Also multiple sections
// in an object.
//
#pragma db namespace table("t6_")
namespace test6
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : n (n_), sn (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    int n;

    #pragma db load(lazy)
    odb::section s1;

    #pragma db load(lazy)
    odb::section s2;

    #pragma db section(s1)
    int sn;

    #pragma db section(s1)
    std::string ss;

    #pragma db section(s2)
    std::vector<int> sv;
  };
}

// Test sections and reuse inheritance.
//
#pragma db namespace table("t7_")
namespace test7
{
  #pragma db object abstract
  struct base
  {
    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy)
    odb::section s1; // Empty section.

    #pragma db load(lazy)
    odb::section s2;

    #pragma db section(s2)
    int sn2;
  };

  #pragma db object abstract
  struct interm: base
  {
    // Section s1 is still empty.

    #pragma db section(s2)
    std::string ss2;
  };

  #pragma db object
  struct derived: interm
  {
    #pragma db section(s1)
    int sn1;
  };

  #pragma db object
  struct object: derived
  {
    object (int n = 999, const std::string& s = "xxx", bool b = false)
    {
      sn1 = sn2 = n;
      ss1 = ss2 = s;
      sb2 = b;
    }

    #pragma db section(s1)
    std::string ss1;

    #pragma db section(s2)
    bool sb2;
  };
}

// Test readonly and inverse section members.
//
#pragma db namespace table("t8_")
namespace test8
{
  struct object1;

  #pragma db object session
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx", object1* p_ = 0)
        : n (n_), sn (n_), ss (s_), sp (p_) {}

    #pragma db id auto
    unsigned long id;

    int n;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s) readonly
    int sn;

    #pragma db section(s)
    std::string ss;

    #pragma db inverse(p) section(s)
    object1* sp;
  };

  #pragma db object session
  struct object1
  {
    object1 (object* p_ = 0): p (p_) {}
    ~object1 () {delete p;}

    #pragma db id auto
    unsigned long id;

    object* p;
  };
}

// Test object without any columns to load or update.
//
#pragma db namespace table("t9_")
namespace test9
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx"): sn (n_), ss (s_) {}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy) update(manual)
    odb::section s;

    #pragma db section(s)
    int sn;

    #pragma db section(s)
    std::string ss;
  };
}

// Test section without any columns or containers to update.
//
#pragma db namespace table("t10_")
namespace test10
{
  #pragma db object
  struct object
  {
    object (int n_ = 999): n (n_), sn (n_) {}

    #pragma db id auto
    unsigned long id;

    int n;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s) readonly
    int sn;
  };
}

// Test section with composite member.
//
#pragma db namespace table("t11_")
namespace test11
{
  #pragma db value
  struct comp
  {
    comp (int n_, const std::string& s_): s (s_) {v.push_back (n_);}

    std::string s;
    std::vector<int> v;
  };

  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : n (n_), sn (n_), sc (n_, s_) {}

    #pragma db id auto
    unsigned long id;

    int n;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s)
    int sn;

    #pragma db section(s)
    comp sc;
  };
}

// Test change state restoration on transaction rollback.
//
#pragma db namespace table("t12_")
namespace test12
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy) update(change)
    odb::section s;

    #pragma db section(s)
    int sn;

    int n;

    #pragma db section(s)
    std::string ss;
  };
}

// Test section accessor/modifier.
//
#pragma db namespace table("t13_")
namespace test13
{
  #pragma db object
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : n (n_), sn (n_), ss (s_) {}

    #pragma db id auto
    unsigned long id;

    int n;

    #pragma db section(s_)
    int sn;

    #pragma db section(s_)
    std::string ss;

  public:
    const odb::section&
    s () const {return s_;}

    odb::section&
    rw_s () {return s_;}

  private:
    #pragma db load(lazy) update(manual)
    odb::section s_;
  };
}

// Test LOB in section streaming, column re-ordering.
//
#pragma db namespace table("t14_")
namespace test14
{
  #pragma db object
  struct object
  {
    object (unsigned long id_ = 0, int n_ = 999, const std::string& s_ = "xxx")
        : id (id_), sb (s_.begin (), s_.end ()), sn (n_), n (n_) {}

    #pragma db id
    unsigned long id;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s) type(BLOB_TYPE)
    std::vector<char> sb; // Comes before sn.

    #pragma db section(s)
    int sn;

    int n;
  };
}

// Test sections and optimistic concurrency.
//
#pragma db namespace table("t15_")
namespace test15
{
  #pragma db object optimistic
  struct object
  {
    object (int n_ = 999, const std::string& s_ = "xxx")
        : sn (n_), n (n_), ss (s_) {sv.push_back (n_);}

    #pragma db id auto
    unsigned long id;

    #pragma db version mssql:type("ROWVERSION")
    unsigned long long v;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s)
    int sn;

    int n;

    #pragma db section(s)
    std::string ss;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test container-only sections and optimistic concurrency.
//
#pragma db namespace table("t16_")
namespace test16
{
  #pragma db object optimistic
  struct object
  {
    object (int n = 999) {sv.push_back (n);}

    #pragma db id auto
    unsigned long id;

    #pragma db version // mssql:type("ROWVERSION")
    unsigned long long v;

    #pragma db load(lazy)
    odb::section s;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test reuse-inheritance, sections, and optimistic concurrency.
//
#pragma db namespace table("t17_")
namespace test17
{
  #pragma db object optimistic sectionable abstract
  struct root
  {
    #pragma db id auto
    unsigned long id;

    #pragma db version
    unsigned long long v;
  };

  #pragma db object
  struct base: root
  {
  };

  #pragma db object
  struct object: base
  {
    object (int n = 999): s1n (n) {s2v.push_back (n);}

    #pragma db load(lazy)
    odb::section s1;

    #pragma db section(s1)
    int s1n;

    #pragma db load(lazy)
    odb::section s2;

    #pragma db section(s2)
    std::vector<int> s2v;
  };
}

// Test change-updated section and change-tracking container.
//
#pragma db namespace table("t18_")
namespace test18
{
  #pragma db object
  struct object
  {
    object (int n = 999): sn (n) {sv.push_back (n);}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy) update(change)
    odb::section s;

    #pragma db section(s)
    int sn;

    #pragma db section(s)
    odb::vector<int> sv;
  };
}

#endif // TEST_HXX
