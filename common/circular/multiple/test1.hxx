// file      : common/circular/multiple/test1.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST1_HXX
#define TEST1_HXX

#include <odb/core.hxx>

#pragma db object
struct derived;

#pragma db object polymorphic
struct base
{
  virtual ~base () {}

  #pragma db id
  unsigned long id_;

  derived* d_;
};

#ifdef ODB_COMPILER
#  include "test2.hxx"
#endif

#endif // TEST1_HXX
