// file      : sqlite/transaction/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#pragma db object
struct object1
{
  #pragma db id
  unsigned long id_;
};

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  odb::lazy_ptr<object1> p;
};

#endif // TEST_HXX
