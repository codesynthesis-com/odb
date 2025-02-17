// file      : common/wrapper/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <memory> // std::unique_ptr
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

using odb::nullable;

// Test 1: simple values.
//
#pragma db namespace table("t1_")
namespace test1
{
  typedef nullable<std::string> nullable_string;

  typedef std::unique_ptr<int> num_uptr;
  typedef std::unique_ptr<std::string> str_uptr;
  typedef std::shared_ptr<std::string> str_sptr;

  #pragma db object table("obj1")
  struct object1
  {
    #pragma db id auto
    unsigned long id_;

    num_uptr num;

    #pragma db null
    str_uptr str;

    nullable_string nstr;
    std::vector<nullable_string> nstrs;
  };

  #pragma db object
  struct object2
  {
    #pragma db id auto
    unsigned long id_;

    #pragma db null
    str_sptr sstr;

    #pragma db value_null
    std::vector<str_sptr> sstrs;
  };
}

//
// Composite values.
//

#pragma db value
struct comp1
{
  comp1 (): num (0) {}
  comp1 (const std::string& s, int n): str (s), num (n) {}

  std::string str;
  int num;
};

inline bool
operator== (const comp1& x, const comp1& y)
{
  return x.str == y.str && x.num == y.num;
}


#pragma db value
struct comp2
{
  comp2 () {}
  comp2 (const std::string& s, int n): str (s), num (n) {}

  std::string str;
  int num;

  std::vector<std::string> strs;
};

inline bool
operator== (const comp2& x, const comp2& y)
{
  return x.str == y.str && x.num == y.num && x.strs == y.strs;
}

struct comp3;

typedef std::unique_ptr<comp1> comp1_uptr;
typedef std::unique_ptr<comp2> comp2_uptr;
typedef std::unique_ptr<comp3> comp3_uptr;

#pragma db object
struct comp_object
{
  #pragma db id auto
  unsigned long id_;

  comp1_uptr                    c1;  // Wrapped comp value.
  std::vector<nullable<comp1> > vc1; // Container of wrapped comp values.
  comp2_uptr                    c2;  // Container inside wrapped comp value.
};

// This one is just a compilation test to cover more convolute cases.
//
#pragma db value
struct comp3: comp2
{
  comp1_uptr c1;
  std::vector<nullable<comp1> > vc1;
};

#pragma db object
struct comp_object2
{
  #pragma db id auto
  unsigned long id_;

  comp3_uptr c3;
};

//
// Containers.
//

typedef std::unique_ptr<std::vector<int>> nums_uptr;
typedef std::unique_ptr<std::vector<std::string>> strs_uptr;

#pragma db value
struct cont_comp
{
  int num;
  strs_uptr strs;
};

inline bool
operator== (const cont_comp& x, const cont_comp& y)
{
  return x.num == y.num && *x.strs == *y.strs;
}

#pragma db object
struct cont_object
{
  #pragma db id auto
  unsigned long id_;

  nums_uptr nums; // Wrapped container.
  cont_comp c;    // Wrapped container in comp value.
};

// Test composite NULL values.
//
#pragma db namespace table("t5_")
namespace test5
{
  #pragma db value
  struct base
  {
    base () {}
    base (int n): num (n) {}

    int num = 0;
  };

  inline bool
  operator== (const base& x, const base& y)
  {
    return x.num == y.num;
  }

  #pragma db value
  struct comp: base
  {
    comp () {}
    comp (int n, const std::string s): base (n), str (s), extra (n + 1) {}

    std::string str;
    base extra;

    odb::nullable<int> always_null;
  };

  inline bool
  operator== (const comp& x, const comp& y)
  {
    return static_cast<const base&> (x) == y &&
      x.str == y.str && x.extra == y.extra;
  }

  #pragma db object
  struct object
  {
    #pragma db id auto
    unsigned long id;

    #pragma db null
    std::unique_ptr<comp> p;

    odb::nullable<comp> n;

    std::vector< odb::nullable<comp> > v;
  };
}

#endif // TEST_HXX
