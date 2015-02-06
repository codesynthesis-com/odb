// file      : common/section/polymorphism/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/section.hxx>

// Test basic polymorphic section functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object polymorphic abstract
  struct root
  {
    root (int n): rs1n (n), rs2n (n), rs4n (n) {rs2v.push_back (n);}
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    // rs1: override in base and derived
    //
    #pragma db load(lazy) update(change)
    odb::section rs1;

    #pragma db section(rs1)
    int rs1n;

    // rs2: no override
    //
    #pragma db load(lazy)
    odb::section rs2;

    #pragma db section(rs2)
    int rs2n;

    #pragma db section(rs2)
    std::vector<int> rs2v;

    // rs3: empty
    //
    #pragma db load(lazy)
    odb::section rs3;

    // rs4: override "gap"
    //
    #pragma db load(lazy)
    odb::section rs4;

    #pragma db section(rs4)
    int rs4n;
  };

  #pragma db object
  struct base: root
  {
    base (int n = 999, const std::string& s = "xxx")
        : root (n), rs1s (s), bs1n (n) {rs3v.push_back (n);}

    // rs1
    //
    #pragma db section(rs1)
    std::string rs1s;

    // rs3
    //
    #pragma db section(rs3)
    std::vector<int> rs3v;

    // bs1: override in derived
    //
    #pragma db load(lazy)
    odb::section bs1;

    #pragma db section(bs1)
    int bs1n;
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999, const std::string& s = "xxx", bool b = false)
        : base (n, s), rs1b (b), rs3n (n), rs4s (s), bs1s (s), ds1n (n)
    {rs1v.push_back (n);}

    // rs1
    //
    #pragma db section(rs1)
    bool rs1b;

    #pragma db section(rs1)
    std::vector<int> rs1v;

    // rs3
    //
    #pragma db section(rs3)
    int rs3n;

    // rs4
    //
    #pragma db section(rs4)
    std::string rs4s;

    // bs1
    //
    #pragma db section(bs1)
    std::string bs1s;

    // ds1: no override
    //
    #pragma db load(lazy)
    odb::section ds1;

    #pragma db section(ds1)
    int ds1n;
  };
}

// Test empty section and override "gap".
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db object polymorphic abstract
  struct root
  {
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    #pragma db load(lazy)
    odb::section s;
  };

  #pragma db object abstract
  struct base: root
  {
    // The "gap".
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999): sn (n) {sv.push_back (n);}

    #pragma db section(s)
    int sn;

    #pragma db section(s)
    std::vector<int> sv;
  };
}

// Test value-only/container-only base/override combinations.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db object polymorphic
  struct root
  {
    root (int n = 999)
        : s1n (n), s2n (n) {s3v.push_back (n); s4nv.push_back (n);}
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    // value/value
    //
    #pragma db load(lazy)
    odb::section s1;

    #pragma db section(s1)
    int s1n;

    // value/container
    //
    #pragma db load(lazy)
    odb::section s2;

    #pragma db section(s2)
    int s2n;

    // container/value
    //
    #pragma db load(lazy)
    odb::section s3;

    #pragma db section(s3)
    std::vector<int> s3v;

    // container/container
    //
    #pragma db load(lazy)
    odb::section s4;

    #pragma db section(s4)
    std::vector<int> s4nv;
  };

  #pragma db object
  struct base: root
  {
    base (int n = 999): root (n) {}

    // The "gap".
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999, const std::string& s = "xxx")
        : base (n), s1s (s), s3n (n) {s2v.push_back (n); s4sv.push_back (s);}

    #pragma db section(s1)
    std::string s1s;

    #pragma db section(s2)
    std::vector<int> s2v;

    #pragma db section(s3)
    int s3n;

    #pragma db section(s4)
    std::vector<std::string> s4sv;
  };
}

// Test basic polymorphic optimistic section functionality.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object polymorphic optimistic abstract sectionable
  struct root
  {
    root (int n): rs1n (n), rs2n (n), rs3n (n), rs4n (n) {}
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    #pragma db version
    unsigned long long v;

    // rs1: readwrite, override
    //
    #pragma db load(lazy) update(change)
    odb::section rs1;

    #pragma db section(rs1)
    int rs1n;

    // rs2: readonly, no override
    //
    #pragma db load(lazy)
    odb::section rs2;

    #pragma db section(rs2)
    const int rs2n;

    // rs3: readonly, readonly override
    //
    #pragma db load(lazy)
    odb::section rs3;

    #pragma db section(rs3)
    const int rs3n;

    // rs4: readonly, readwrite override
    //
    #pragma db load(lazy)
    odb::section rs4;

    #pragma db section(rs4)
    const int rs4n;
  };

  #pragma db object
  struct base: root
  {
    base (int n = 999, const std::string& s = "xxx")
        : root (n), rs1s (s), rs4s (s) {}

    // rs1
    //
    #pragma db section(rs1)
    std::string rs1s;

    // rs4
    //
    #pragma db section(rs4)
    std::string rs4s;

    // bs2: empty, readwrite override
    //
    #pragma db load(lazy)
    odb::section bs1;
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999, const std::string& s = "xxx", bool b = false)
        : base (n, s), rs1b (b), rs3s (s), bs1n (n)
    {
      rs1v.push_back (n);
      rs4v.push_back (n);
      ds1v.push_back (n);
    }

    // rs1
    //
    #pragma db section(rs1)
    bool rs1b;

    #pragma db section(rs1)
    std::vector<int> rs1v;

    // rs3
    //
    #pragma db section(rs3)
    const std::string rs3s;

    // rs4
    //
    #pragma db section(rs4)
    std::vector<int> rs4v;

    // bs1
    //
    #pragma db section(bs1)
    int bs1n;

    // ds1: readwrite
    //
    #pragma db load(lazy)
    odb::section ds1;

    #pragma db section(ds1)
    std::vector<int> ds1v;
  };
}

// Test polymorphic optimistic readonly/empty to readwrite section override.
//
#pragma db namespace table("t5_")
namespace test5
{
  #pragma db object polymorphic optimistic abstract
  struct root
  {
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    #pragma db version
    unsigned long long v;

    #pragma db load(lazy) update(change)
    odb::section s;
  };

  #pragma db object
  struct base: root
  {
    // The "gap".
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999): sn (n) {}

    #pragma db section(s)
    int sn;
  };
}

// Test polymorphic optimistic readonly/empty to readwrite section override,
// eager-loaded case.
//
#pragma db namespace table("t6_")
namespace test6
{
  #pragma db object polymorphic optimistic abstract
  struct root
  {
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    #pragma db version
    unsigned long long v;

    #pragma db update(change)
    odb::section s;
  };

  #pragma db object abstract
  struct base: root
  {
    // The "gap".
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999): sn (n) {}

    #pragma db section(s)
    int sn;
  };
}

// Test polymorphic optimistic section added in derived.
//
#pragma db namespace table("t7_")
namespace test7
{
  #pragma db object polymorphic optimistic sectionable
  struct root
  {
    virtual ~root () {}

    #pragma db id auto
    unsigned long id;

    #pragma db version
    unsigned long long v;
  };

  #pragma db object
  struct base: root
  {
    #pragma db load(lazy) update(change)
    odb::section s;
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999): sn (n) {}

    #pragma db section(s)
    int sn;
  };
}

// Test reuse/polymorphic inheritance and optimistic mix.
//
#pragma db namespace table("t8_")
namespace test8
{
  #pragma db object optimistic sectionable abstract
  struct root
  {
    #pragma db id auto
    unsigned long id;

    #pragma db version
    unsigned long long v;
  };

  #pragma db object polymorphic sectionable
  struct base: root
  {
    virtual ~base () {}
  };

  #pragma db object
  struct derived: base
  {
    derived (int n = 999): sn (n) {}

    #pragma db load(lazy) update(change)
    odb::section s;

    #pragma db section(s)
    int sn;
  };
}

#endif // TEST_HXX
