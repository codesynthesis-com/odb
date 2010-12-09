// file      : common/relationship/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <set>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "tr1-memory.hxx"

#include <odb/core.hxx>

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

// auto_ptr
//
#pragma db object pointer(std::auto_ptr<obj2>)
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

// tr1::shared_ptr
//
#ifdef HAVE_TR1_MEMORY
struct obj3;
typedef std::tr1::shared_ptr<obj3> obj3_ptr;

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
  std::auto_ptr<obj2> o2;
#ifdef HAVE_TR1_MEMORY
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
#ifdef HAVE_TR1_MEMORY
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
