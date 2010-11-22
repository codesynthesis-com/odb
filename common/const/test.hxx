// file      : common/const/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <memory>
#include <odb/core.hxx>

struct obj1;
struct obj2;

typedef obj1* obj1_ptr;
typedef const obj1* obj1_cptr;

typedef std::auto_ptr<obj2> obj2_ptr;
typedef std::auto_ptr<const obj2> obj2_cptr;

#pragma db object pointer (obj1_ptr)
struct obj1
{
  obj1 () {}
  obj1 (int i): id (i) {}

  #pragma db id
  int id;

  void f () {}
  void cf () const {}
};

#pragma db object pointer (obj2_ptr)
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

  obj1_cptr o1;
  obj2_cptr o2;
};

#endif // TEST_HXX
