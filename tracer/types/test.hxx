// file      : tracer/types/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
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

#pragma odb object
struct object1
{
  typedef int int_type;

  #pragma odb id
  int_type id_;
};

#pragma odb object
struct object2
{
  #pragma odb id
  num_type num_;
};

#endif // TEST_HXX
