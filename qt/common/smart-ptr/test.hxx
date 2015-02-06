// file      : qt/common/smart-ptr/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <QtCore/QSharedPointer>

#include <odb/core.hxx>
#include <odb/qt/lazy-ptr.hxx>

struct obj;

#pragma db object
struct cont
{
  cont ()
  {
  }

  cont (unsigned long id)
      : id (id)
  {
  }

  #pragma db id
  unsigned long id;

  typedef std::vector<QLazyWeakPointer<obj> > obj_list;

  #pragma db inverse(c) value_not_null
  obj_list o;
};

#pragma db object
struct obj
{
  obj ()
  {
  }

  obj (unsigned long id)
      : id (id)
  {
  }

  #pragma db id
  unsigned long id;

  #pragma db not_null
  QLazySharedPointer<cont> c;
};

// Test QSharedPointer as a value wrapper.
//
#pragma db object
struct obj2
{
  obj2 ()
  {
  }

  obj2 (unsigned long id)
      : id (id)
  {
  }

  #pragma db id
  unsigned long id;

  #pragma db null
  QSharedPointer<unsigned long> num;
};

#endif // TEST_HXX
