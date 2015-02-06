// file      : mysql/custom/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>

// Map GEOMETRY MySQL type to the point C++ struct. The other half
// of this mapping is in traits.hxx (value_traits<point, id_string>).
//
#pragma db map type("GEOMETRY")        \
               as("VARCHAR(256)")      \
               to("GeomFromText((?))") \
               from("AsText((?))")

#pragma db value type("GEOMETRY")
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

#pragma db object
struct object
{
  object () {}
  object (unsigned long id_) : id (id_) {}

  #pragma db id
  unsigned long id;

  point p;
  std::vector<point> pv;

  bool
  operator== (const object& y) const
  {
    return id == y.id && p == y.p && pv == y.pv;
  }
};

#endif // TEST_HXX
