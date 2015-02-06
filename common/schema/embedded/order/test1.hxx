// file      : common/schema/embedded/order/test1.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST1_HXX
#define TEST1_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object polymorphic
struct base
{
  virtual
  ~base () {}

  #pragma db auto id
  unsigned long id;

  std::string str;
};

#endif // TEST1_HXX
