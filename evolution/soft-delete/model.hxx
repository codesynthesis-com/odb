// file      : evolution/soft-delete/model.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include <common/config.hxx> // DATABASE_XXX

#pragma db model version(1, MODEL_VERSION)

#define MODEL_NAMESPACE_IMPL(V) v##V
#define MODEL_NAMESPACE(V) MODEL_NAMESPACE_IMPL(V)

namespace MODEL_NAMESPACE(MODEL_VERSION)
{
  // Test soft-deleted objects.
  //
  #pragma db namespace table("t1_")
  namespace test1
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db object(object) deleted(3)
#endif
  }

  // SQLite doesn't support dropping of columns.
  //
#ifndef DATABASE_SQLITE

  // Test basic soft-deleted member logic.
  //
  #pragma db namespace table("t2_")
  namespace test2
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      std::string str;
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test empty statement handling (INSERT, UPDATE).
  //
  #pragma db namespace table("t3_")
  namespace test3
  {
    #pragma db object
    struct object
    {
      #pragma db id auto
      unsigned long id;

      std::string str;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test empty statement handling (SELECT in polymorphic loader).
  //
  #pragma db namespace table("t4_")
  namespace test4
  {
    #pragma db object polymorphic
    struct base
    {
      virtual
      ~base () {}
      base (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;
    };

    #pragma db object
    struct object: base
    {
      object (unsigned long id = 0): base (id) {}

      std::string str;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

#endif // DATABASE_SQLITE

}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
