// file      : common/const-object/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <memory>
#include <odb/core.hxx>

#pragma db object pointer (obj1*) session
struct obj1
{
  obj1 () {}
  obj1 (int i): id (i) {}

  #pragma db id
  int id;

  void f () {}
  void cf () const {}
};

#pragma db object pointer (std::auto_ptr<obj2>)
struct obj2
{
  obj2 () {}
  obj2 (int i): id (i) {}

  #pragma db id
  int id;

  void f () {}
  void cf () const {}
};

#pragma db object
struct aggr
{
  aggr (int i): id (i), o1 (0) {}
  aggr (): o1 (0) {}
  ~aggr () {delete o1;}

  #pragma db id
  int id;

  const obj1* o1;
  std::auto_ptr<const obj2> o2;
};

#endif // TEST_HXX
