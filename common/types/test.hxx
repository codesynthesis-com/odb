// file      : common/types/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

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
  typedef num_wrap this_type; // Name that we should not use.
#endif

  num_wrap () {}
  num_wrap (X v): v_ (v) {}
  operator X () const {return v_;}

  X v_;
};

#pragma db object
struct object3
{
  #pragma db id type("INTEGER")
  num_wrap<long long> num_; // Use long long to avoid warnings.
};

#endif // TEST_HXX
