// file      : qt/mysql/core/test.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <QString>
#include <QDateTime>

#pragma db object
struct time_punchcard
{
  #pragma db id auto
  unsigned long id;

  QDateTime date_time;
};

#pragma db object
struct person
{
  bool
  operator== (const person& x) const
  {
    return name == x.name &&
      date_of_birth == x.date_of_birth &&
      time_of_birth == x.time_of_birth;
  }

  #pragma db id
  QString name;

  #pragma db type("DATE NOT NULL")
  QDate date_of_birth;
  QTime time_of_birth;
};

#endif // TEST_HXX
