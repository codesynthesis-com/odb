// file      : qt/common/containers/change-tracking/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <memory>

#ifdef HAVE_CXX11
#  include <utility> // std::move
#endif

#include <QtCore/QString>

#include <odb/core.hxx>
#include <odb/qt/list.hxx>

#pragma db object pointer(std::auto_ptr)
struct object
{
  object () {}
  object (const QString& id): id_ (id) {}

#ifdef HAVE_CXX11
  object (const object& x): id_ (x.id_), i (x.i), s (x.s) {}
  object (object&& x): id_ (std::move (x.id_)), i (x.i), s (std::move (x.s)) {}
#endif

  #pragma db id
  QString id_;

  unsigned int i;

  QOdbList<QString> s;

  inline bool
  operator== (const object& o) {return id_ == o.id_ && i == o.i && s == o.s;}
};

#endif // TEST_HXX
