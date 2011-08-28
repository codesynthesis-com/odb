// file      : common/wrapper/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_TR1_MEMORY

#include <string>
#include <memory> // std::auto_ptr
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#ifdef HAVE_TR1_MEMORY
#  include <odb/tr1/memory.hxx>
#endif

using odb::nullable;

//
// Simple values.
//

typedef nullable<std::string> nullable_string;

#ifdef HAVE_TR1_MEMORY
typedef std::tr1::shared_ptr<std::string> tr1_nullable_string;
#endif

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  std::auto_ptr<int> num;

  #pragma db null
  std::auto_ptr<std::string> str;

  nullable_string nstr;
  std::vector<nullable_string> nstrs;

#ifdef HAVE_TR1_MEMORY
  #pragma db null
  tr1_nullable_string tr1_str;

  #pragma db value_null
  std::vector<tr1_nullable_string> tr1_strs;
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

#pragma db object
struct comp_object
{
  #pragma db id auto
  unsigned long id_;

  std::auto_ptr<comp1> c1;           // Wrapped comp value.
  std::vector<nullable<comp1> > vc1; // Container of wrapped comp values.
  std::auto_ptr<comp2> c2;           // Container inside wrapped comp value.
};

// This one is just a compilation test to cover more convolute cases.
//
#pragma db value
struct comp3: comp2
{
  std::auto_ptr<comp1> c1;
  std::vector<nullable<comp1> > vc1;
};

#pragma db object
struct comp_object2
{
  #pragma db id auto
  unsigned long id_;

  std::auto_ptr<comp3> c3;
};

//
// Containers.
//

#pragma db value
struct cont_comp
{
  int num;
  std::auto_ptr<std::vector<std::string> > strs;
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

  std::auto_ptr<std::vector<int> > vi; // Wrapped container.
  cont_comp c;                         // Wrapped container in comp value.
};

#endif // TEST_HXX
