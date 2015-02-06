// file      : common/default/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <odb/core.hxx>

enum color {red, green, blue};

#pragma db value(unsigned long) default(0)

#pragma db object
struct object
{
  #pragma db id
  unsigned long obj_id;

  // NULL.
  //
  #pragma db null default(null)
  unsigned long null;

  // Boolean.
  //
  #pragma db default(true)
  bool b;

  // Integers.
  //
  #pragma db default(1234)
  unsigned long pi;

  #pragma db default(-1234)
  long ni;

  // 0 default taken from the type.
  unsigned long zi;

  // Floats.
  //
  #pragma db default(1.234)
  double pf;

  #pragma db default(-1.234)
  double nf;

  #pragma db default(0.0)
  double zf;

  #pragma db default(1.123e+10)
  double sf;

  // Strings. MySQL doesn't support default values on TEXT
  // columns, so make the type VARCHAR.
  //
  #pragma db type("VARCHAR(64)") default("Someone's string")
  std::string str;

  // Enums.
  //
  #pragma db default(green)
  color e;
};

#endif // TEST_HXX
