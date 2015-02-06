// file      : qt/mssql/basic/test.hxx
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
      id_ == x.id_ &&
      sstr_ == x.sstr_ &&
      lstr_ == x.lstr_ &&
      snstr_ == x.snstr_ &&
      lnstr_ == x.lnstr_ &&
      sbuf_ == x.sbuf_ &&
      lbuf_ == x.lbuf_;
  }

  #pragma db id
  QString id_;

  QString sstr_;

  #pragma db type ("VARCHAR(max)")
  QString lstr_;

  #pragma db type ("NVARCHAR(512)")
  QString snstr_;

  #pragma db type ("NVARCHAR(max)")
  QString lnstr_;

  #pragma db type ("VARBINARY(512)")
  QByteArray sbuf_;

  QByteArray lbuf_;
};

#endif // TEST_HXX
