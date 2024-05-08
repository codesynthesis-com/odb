// file      : qt/common/containers/change-tracking/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <memory>
#include <utility> // std::move

#include <QtCore/QString>

#include <odb/core.hxx>
#include <odb/qt/list.hxx>

#pragma db object pointer(std::unique_ptr)
struct object
{
  object () {}
  object (const QString& id): id_ (id) {}

  object (const object& x): id_ (x.id_), i (x.i), s (x.s) {}
  object (object&& x): id_ (std::move (x.id_)), i (x.i), s (std::move (x.s)) {}

  #pragma db id
  QString id_;

  unsigned int i;

  QOdbList<QString> s;

  inline bool
  operator== (const object& o) const
  {
    return id_ == o.id_ && i == o.i && s == o.s;
  }
};

#endif // TEST_HXX
