// file      : qt/oracle/basic/test.hxx
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
      varchar2 == x.varchar2 &&
      char_ == x.char_ &&
      nchar == x.nchar &&
      nvarchar2 == x.nvarchar2 &&
      clob == x.clob &&
      nclob == x.nclob &&
      blob == x.blob &&
      raw == x.raw;
  }

  #pragma db id
  QString varchar2;

  #pragma db type ("CHAR(13)")
  QString char_;

  #pragma db type ("NCHAR(13)")
  QString nchar;

  #pragma db type ("NVARCHAR2(512)")
  QString nvarchar2;

  #pragma db type ("CLOB")
  QString clob;

  #pragma db type ("NCLOB")
  QString nclob;

  QByteArray blob;

  #pragma db type ("RAW(128)")
  QByteArray raw;
};

#endif // TEST_HXX
