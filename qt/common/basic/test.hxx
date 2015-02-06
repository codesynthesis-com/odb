// file      : qt/common/basic/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <QtCore/QUuid>

#include <odb/core.hxx>

#pragma db object
struct object
{
  object () {}
  object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  QUuid uuid_;
  QUuid null_;

  #pragma db not_null
  QUuid zero_;

  bool operator== (const object& x) const
  {
    return id_ == x.id_ &&
      uuid_ == x.uuid_ &&
      null_ == x.null_ &&
      zero_ == x.zero_;
  }
};

#endif // TEST_HXX
