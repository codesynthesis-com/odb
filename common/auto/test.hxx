// file      : common/auto/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma odb object
struct object
{
  object (const std::string& str)
      : id_ (1), str_ (str)
  {
  }

  #pragma odb auto id
  unsigned long id_;
  std::string str_;

private:
  object ()
  {
  }

  friend class odb::access;
};

#endif // TEST_HXX
