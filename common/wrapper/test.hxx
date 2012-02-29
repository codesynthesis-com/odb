// file      : common/wrapper/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <string>
#include <memory> // std::auto_ptr
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#endif

using odb::nullable;

//
// Simple values.
//

typedef nullable<std::string> nullable_string;

#ifdef HAVE_CXX11
typedef std::unique_ptr<int> num_uptr;
typedef std::unique_ptr<std::string> str_uptr;
typedef std::shared_ptr<std::string> str_sptr;
#else
typedef std::auto_ptr<int> num_uptr;
typedef std::auto_ptr<std::string> str_uptr;
#  ifdef HAVE_TR1_MEMORY
typedef std::tr1::shared_ptr<std::string> str_sptr;
#  endif
#endif

#pragma db object table("obj")
struct object
{
  #pragma db id auto
  unsigned long id_;

  num_uptr num;

  #pragma db null
  str_uptr str;

  nullable_string nstr;
  std::vector<nullable_string> nstrs;

#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
  #pragma db null
  str_sptr sstr;

  #pragma db value_null
  std::vector<str_sptr> sstrs;
#endif
};

//
// Composite values.
//

#pragma db value
struct comp1
{
  comp1 () {}
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

#ifdef HAVE_CXX11
typedef std::unique_ptr<comp1> comp1_uptr;
typedef std::unique_ptr<comp2> comp2_uptr;
typedef std::unique_ptr<comp3> comp3_uptr;
#else
typedef std::auto_ptr<comp1> comp1_uptr;
typedef std::auto_ptr<comp2> comp2_uptr;
typedef std::auto_ptr<comp3> comp3_uptr;
#endif

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

#ifdef HAVE_CXX11
typedef std::unique_ptr<std::vector<int>> nums_uptr;
typedef std::unique_ptr<std::vector<std::string>> strs_uptr;
#else
typedef std::auto_ptr<std::vector<int> > nums_uptr;
typedef std::auto_ptr<std::vector<std::string> > strs_uptr;
#endif

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

#endif // TEST_HXX
