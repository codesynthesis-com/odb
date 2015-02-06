// file      : common/no-id/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object no_id
struct object
{
  object () {}
  object (unsigned long n, const std::string& s): num (n), str (s) {}

  unsigned long num;
  std::string str;
};

#endif // TEST_HXX
