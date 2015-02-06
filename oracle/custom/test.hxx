// file      : oracle/custom/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>

// Map Numbers VARRAY Oracle type to std::vector<int>. This type is defined
// in the custom.sql file along with two helper functions that convert
// between Numbers and its string representation. The other half of this
// mapping is in traits.hxx (value_traits<std::vector<int>, id_string>).
//
#pragma db map type("Numbers")                               \
               as("VARCHAR2(1500)")                          \
               to("CAST(string_to_numbers((?)) AS Numbers)") \
               from("numbers_to_string((?))")

#pragma db object
struct object
{
  object () {}
  object (unsigned long id_) : id (id_) {}

  #pragma db id
  unsigned long id;

  #pragma db type("Numbers")
  std::vector<int> iv;

  bool
  operator== (const object& y) const
  {
    return id == y.id && iv == y.iv;
  }
};

#endif // TEST_HXX
