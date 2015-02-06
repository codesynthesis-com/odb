// file      : common/relationship/basics/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <set>
#include <map>
#include <vector>
#include <string>
#include <memory>

#include <odb/core.hxx>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#endif

// Raw pointer.
//
#pragma db object pointer(obj1*)
struct obj1
{
  obj1 () {}
  obj1 (const std::string& i, const std::string& s): id (i), str (s) {}

  #pragma db id
  std::string id;
  std::string str;
};

inline bool
operator== (const obj1& x, const obj1& y)
{
  return x.id == y.id && x.str == y.str;
}

// vector
//
typedef std::vector<obj1*> obj1_vec;

inline bool
operator== (const obj1_vec& x, const obj1_vec& y)
{
  if (x.size () != y.size ())
    return false;

  for (obj1_vec::size_type i (0); i < x.size (); ++i)
    if (!(x[i] ? (y[i] && *x[i] == *y[i]) : !y[i]))
      return false;

  return true;
}

// set
//
struct obj1_cmp
{
  bool
  operator() (obj1* x, obj1* y) const
  {
    return (!x || !y) ? x < y : x->id < y->id;
  }
};

typedef std::set<obj1*, obj1_cmp> obj1_set;

inline bool
operator== (const obj1_set& x, const obj1_set& y)
{
  if (x.size () != y.size ())
    return false;

  for (obj1_set::const_iterator i (x.begin ()); i != x.end (); ++i)
  {
    obj1_set::const_iterator j (y.find (*i));

    if (j == y.end ())
      return false;

    obj1* x (*i);
    obj1* y (*j);

    if (!(x ? (y && *x == *y) : !y))
      return false;
  }

  return true;
}

// map
//
typedef std::map<int, obj1*> obj1_map;

inline bool
operator== (const obj1_map& x, const obj1_map& y)
{
  if (x.size () != y.size ())
    return false;

  for (obj1_map::const_iterator i (x.begin ()); i != x.end (); ++i)
  {
    obj1_map::const_iterator j (y.find (i->first));

    if (j == y.end ())
      return false;

    obj1* x (i->second);
    obj1* y (j->second);

    if (!(x ? (y && *x == *y) : !y))
      return false;
  }

  return true;
}

// auto_ptr/unique_ptr
//
struct obj2;

#ifdef HAVE_CXX11
typedef std::unique_ptr<obj2> obj2_ptr;
#else
typedef std::auto_ptr<obj2> obj2_ptr;
#endif

#pragma db object pointer(obj2_ptr)
struct obj2
{
  obj2 () {}
  obj2 (const std::string& s): str (s) {}

  #pragma db id auto
  unsigned long id;

  std::string str;
};

inline bool
operator== (const obj2& x, const obj2& y)
{
  return x.id == y.id && x.str == y.str;
}

#ifdef HAVE_CXX11
typedef std::vector<obj2_ptr> obj2_vec;

inline bool
operator== (const obj2_vec& x, const obj2_vec& y)
{
  if (x.size () != y.size ())
    return false;

  for (obj2_vec::size_type i (0); i < x.size (); ++i)
    if (!(x[i] ? (y[i] && *x[i] == *y[i]) : !y[i]))
      return false;

  return true;
}
#endif

// shared_ptr
//
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
struct obj3;

#ifdef HAVE_CXX11
typedef std::shared_ptr<obj3> obj3_ptr;
#else
typedef std::tr1::shared_ptr<obj3> obj3_ptr;
#endif

#pragma db object pointer(obj3_ptr)
struct obj3
{
  obj3 () {}
  obj3 (const std::string& s): str (s) {}

  #pragma db id auto
  unsigned long id;

  std::string str;
};

inline bool
operator== (const obj3& x, const obj3& y)
{
  return x.id == y.id && x.str == y.str;
}

// composite
//
#pragma db value
struct comp
{
  comp () {}
  comp (int n, obj3_ptr o): num (n), o3 (o) {}

  int num;
  obj3_ptr o3;
};

inline bool
operator== (const comp& x, const comp& y)
{
  return x.num == y.num &&
    (x.o3 ? (y.o3 && *x.o3 == *y.o3) : !y.o3);
}

typedef std::vector<comp> comp_vec;
#endif

//
//
#pragma db object
struct aggr
{
  aggr (): o1 (0) {}
  aggr (const std::string& s): o1 (0), str (s) {}

  ~aggr ()
  {
    delete o1;

    for (obj1_vec::iterator i (v1.begin ()); i != v1.end (); ++i)
      delete *i;

    for (obj1_set::iterator i (s1.begin ()); i != s1.end (); ++i)
      delete *i;

    for (obj1_map::iterator i (m1.begin ()); i != m1.end (); ++i)
      delete i->second;
  }

  #pragma db id auto
  unsigned long id;

  obj1* o1;

  obj2_ptr o2; // std::auto_ptr or std::unique_ptr
#ifdef HAVE_CXX11
  obj2_vec v2;
#else
  // Dummy containers to get the equivalent DROP TABLE statements.
  //
  std::vector<int> v2;
#endif

#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
  obj3_ptr o3;
  comp c;
  comp_vec cv;
#endif

  obj1_vec v1;
  obj1_set s1;
  obj1_map m1;

  std::string str;

private:
  aggr (const aggr&);
  aggr& operator= (const aggr&);
};

inline bool
operator== (const aggr& x, const aggr& y)
{
  return
    x.id == y.id &&
    (x.o1 ? (y.o1 && *x.o1 == *y.o1) : !y.o1) &&
    (x.o2.get () ? (y.o2.get () && *x.o2 == *y.o2) : !y.o2.get ()) &&
#ifdef HAVE_CXX11
    x.v2 == y.v2 &&
#endif
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
    (x.o3.get () ? (y.o3.get () && *x.o3 == *y.o3) : !y.o3.get ()) &&
    x.c == y.c &&
    x.cv == y.cv &&
#endif
    x.v1 == y.v1 &&
    x.s1 == y.s1 &&
    x.m1 == y.m1 &&
    x.str == y.str;
}

#endif // TEST_HXX
