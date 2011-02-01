// file      : tracer/types/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <iosfwd>

#ifdef ODB_COMPILER
typedef int int_t;
typedef short num_t;
#else
typedef int num_t;
#endif

typedef num_t num_type;

#pragma db object
struct object1
{
  typedef int int_type;

  #pragma db id
  int_type id_;
};

#pragma db object
struct object2
{
  #pragma db id
  num_type num_;
};

// Template-id with "inner" name (compilation test).
//
template <typename X>
struct num_wrap
{
#ifdef ODB_COMPILER
  typedef num_wrap this_type;
#endif

  X v_;
};

template <typename X>
std::ostream&
operator<< (std::ostream& os, const num_wrap<X>& x)
{
  return os << x.v_;
}

template <typename X>
bool
operator== (const num_wrap<X>& x, const num_wrap<X>& y)
{
  return x.v_ == y.v_;
}

#pragma db object
struct object3
{
  #pragma db id
  num_wrap<int> num_;
};

#endif // TEST_HXX
