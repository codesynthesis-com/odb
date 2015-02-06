// file      : common/circular/single/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>

struct derived;

#pragma db object polymorphic
struct base
{
  virtual ~base () {}

  #pragma db id
  unsigned long id_;

  derived* d_;
};

#pragma db object
struct derived: base
{
  base* b_;
};

#endif // TEST_HXX
