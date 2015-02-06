// file      : common/auto/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object
struct object
{
  object (const std::string& str)
      : id_ (1), str_ (str)
  {
  }

  #pragma db auto id
  unsigned long id_;
  std::string str_;

private:
  object ()
  {
  }

  friend class odb::access;
};

// Test the case where the object has just the auto id.
//
#pragma db object
struct auto_only
{
  #pragma db auto id pgsql:type("BIGINT")
  unsigned short id_;
};

#endif // TEST_HXX
