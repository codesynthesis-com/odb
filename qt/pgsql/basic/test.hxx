// file      : qt/pgsql/basic/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>

#pragma db object
struct object
{
  bool
  operator== (const object& x) const
  {
    return
      str == x.str &&
      blob == x.blob;
  }

  #pragma db id
  QString str;

  QByteArray blob;
};

#endif // TEST_HXX
