// file      : sqlite/custom/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

// Map NUMERIC SQLite type to std::string (or any other type that
// provides the value_traits<?, id_string> specialization). By
// default ODB treats NUMERIC as REAL. Note also that we don't
// need to specify to/from conversions since SQLite will convert
// implicitly.
//
#pragma db map type("NUMERIC") as("TEXT")

#pragma db object
struct object
{
  object () {}
  object (unsigned long id_): id (id_) {}

  #pragma db id
  unsigned long id;

  #pragma db value_type("NUMERIC")
  std::vector<std::string> nv;

  bool
  operator== (const object& y) const
  {
    return id == y.id && nv == y.nv;
  }
};

#endif // TEST_HXX
