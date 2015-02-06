// file      : common/schema/namespace/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

// Table names.
//
#pragma db object table("TABLE_EXPLICIT")
struct table_explicit
{
  #pragma db id
  unsigned long id_;
};

#pragma db object
struct table_implicit
{
  #pragma db id
  unsigned long id_;
};

// Column names.
//
#pragma db object
struct column
{
  #pragma db id
  int m1;

  #pragma db column("foo")
  int m2;

  int m_m3;
  int _m4;
  int m5_;
  int m_;
  int m__;
};

// Column types.
//
#pragma db object
struct type
{
  #pragma db id
  std::string id;

  // Test default C++ to DB type mapping.
  //
  bool b;
  char c;
  signed char sc;
  unsigned char uc;
  short s;
  unsigned short us;
  int i;
  unsigned int ui;
  long l;
  unsigned long ul;
  long long ll;
  unsigned long long ull;
  float f;
  double d;
  std::string str;

  #pragma db type("INTEGER")
  bool m1;

  #pragma db transient
  char* m2;
};

// Test database schema (aka database namespace).
//
#ifdef ODB_COMPILER
#if defined (ODB_DATABASE_MYSQL)
//#  define DB_SCHEMA "odb_test"
#  define DB_SCHEMA ""
#elif defined (ODB_DATABASE_SQLITE)
#  define DB_SCHEMA "main"
#elif defined (ODB_DATABASE_PGSQL)
#  define DB_SCHEMA "public"
#elif defined (ODB_DATABASE_ORACLE)
//#  define DB_SCHEMA "ODB_TEST"
#  define DB_SCHEMA ""
#elif defined(ODB_DATABASE_MSSQL)
#  define DB_SCHEMA "dbo"
#elif defined(ODB_DATABASE_COMMON)
#  define DB_SCHEMA "dummy"
#else
#  error unknown database
#endif
#endif

namespace ns {typedef int my_int;} // Original.

#pragma db object table(DB_SCHEMA."object_1")
struct object1
{
  #pragma db id auto
  unsigned long id;

  #pragma db column("str")
  std::string str;
};

inline bool
operator== (const object1& x, const object1& y)
{
  return x.id == y.id && x.str == y.str;
}

#pragma db namespace schema(DB_SCHEMA)
namespace ns // Extension.
{
  #pragma db object
  struct object2
  {
    object2 (): obj1 (0) {}
    ~object2 () {delete obj1;}

    #pragma db id
    std::string id;

    std::vector<unsigned int> nums;
    object1* obj1;
  };

  inline bool
  operator== (const object2& x, const object2& y)
  {
    return x.id == y.id && x.nums == y.nums && *x.obj1 == *y.obj1;
  }
}

#pragma db view object(object1) object(ns::object2)
struct object_view
{
  #pragma db column(ns::object2::id)
  std::string id2;

  std::string str;
};

#pragma db view table(DB_SCHEMA."schema_ns_object_1")
struct table_view
{
  #pragma db column(DB_SCHEMA."schema_ns_object_1"."str")
  std::string str;
};

#endif // TEST_HXX
