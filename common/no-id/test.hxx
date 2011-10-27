// file      : common/no-id/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object id()
struct object
{
  object () {}
  object (unsigned long n, const std::string& s): num (n), str (s) {}

  unsigned long num;
  std::string str;
};

#endif // TEST_HXX
