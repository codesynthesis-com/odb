// file      : pgsql/custom/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

// Map POINT PostgreSQL type to the point C++ struct. The other half
// of this mapping is in traits.hxx (value_traits<point, id_string>).
//
#pragma db map type("POINT") as("TEXT") to("(?)::POINT") from("(?)::TEXT")

#pragma db value type("POINT")
struct point
{
  point () {}
  point (double x_, double y_): x (x_), y (y_) {}

  double x;
  double y;
};

inline bool
operator== (const point& a, const point& b)
{
  return a.x == b.x && a.y == b.y;
}

// Map NUMERIC PostgreSQL type to std::string (or any other type that
// provides the value_traits<?, id_string> specialization).
//
#pragma db map type("NUMERIC *(\\(.+\\))?") \
               as("TEXT")                   \
               to("(?)::NUMERIC$1")         \
               from("(?)::TEXT")

// Map INTEGER[] PostgreSQL type to std::vector<int>. The other half of
// this mapping is in traits.hxx (value_traits<std::vector<int>, id_string>).
//
#pragma db map type("INTEGER *\\[(\\d*)\\]") \
               as("TEXT")                    \
               to("(?)::INTEGER[$1]")        \
               from("(?)::TEXT")

#pragma db object
struct object
{
  object () {}
  object (unsigned long id_) : id (id_) {}

  #pragma db id
  unsigned long id;

  point p;
  std::vector<point> pv;

  #pragma db type("NUMERIC(6, 4)")
  std::string n1;

  #pragma db type("NUMERIC(6)")
  std::string n2;

  #pragma db type("NUMERIC")
  std::string n3;

  #pragma db type("INTEGER [123]")
  std::vector<int> iv;

  bool
  operator== (const object& y) const
  {
    return id == y.id &&
           p == y.p   &&
           pv == y.pv &&
           n1 == y.n1 &&
           n2 == y.n2 &&
           n3 == y.n3 &&
           iv == y.iv;
  }
};

#endif // TEST_HXX
