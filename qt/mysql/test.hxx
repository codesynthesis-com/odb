// file      : qt/mysql/test.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <QString>

#pragma db object
struct person
{
  bool
  operator== (const person& x) const
  {
    return id == x.id && name == x.name;
  }

  #pragma db id auto
  unsigned long id;

  QString name;
};

#endif // TEST_HXX
