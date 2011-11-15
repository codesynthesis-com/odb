// file      : qt/oracle/basic/test.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
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
      varchar == x.varchar &&
      clob == x.clob &&
      nclob == x.nclob &&
      blob == x.blob &&
      raw == x.raw;
  }

  #pragma db id
  QString varchar;

  #pragma db type ("CLOB")
  QString clob;

  #pragma db type ("NCLOB")
  QString nclob;

  QByteArray blob;

  #pragma db type ("RAW(128)")
  QByteArray raw;
};

#endif // TEST_HXX
