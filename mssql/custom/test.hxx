// file      : mssql/types/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
// Map GEOMETRY SQL Server type to the point C++ struct. The other half
// of this mapping is in traits.hxx (value_traits<point, id_string>).
// Note that GEOMETRY is not available in SQL Server 2005.
//
#pragma db map type("GEOMETRY")                       \
               as("VARCHAR(256)")                     \
               to("GEOMETRY::STGeomFromText((?), 0)") \
               from("(?).STAsText()")

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
#endif // SQL Server > 2005

// Map XML SQL Server type to std::string (or any other type that provides
// the value_traits<?, id_long_string> specialization). Note also that
// another alternative would be to interface with the XML data type using
// VARBINARY or NVARCHAR. Here we use implicit string to/from XML conversion,
// however, CAST/CONVERT can be used instead for greater control over
// whitespace handling, etc.
//
#pragma db map type("XML *(\\(.+\\))?") as("VARCHAR(max)")

#pragma db object
struct object
{
  object () {}
  object (unsigned long id_) : id (id_) {}

  #pragma db id
  unsigned long id;

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
  point p;
  std::vector<point> pv;
#endif

  #pragma db type("XML")
  std::string xml;

  bool
  operator== (const object& y) const
  {
    return id == y.id
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      && p == y.p
      && pv == y.pv
#endif
      && xml == y.xml;
  }
};

#endif // TEST_HXX
