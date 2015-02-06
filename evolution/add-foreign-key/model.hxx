// file      : evolution/add-foreign-key/model.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <string>

#include <odb/core.hxx>

#include <common/config.hxx> // DATABASE_XXX

#pragma db model version(1, MODEL_VERSION)

#define MODEL_NAMESPACE_IMPL(V) v##V
#define MODEL_NAMESPACE(V) MODEL_NAMESPACE_IMPL(V)

namespace MODEL_NAMESPACE(MODEL_VERSION)
{
#if MODEL_VERSION == 3
  #pragma db object
  struct object1
  {
    object1 (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;
  };

  #pragma db object
  struct object2
  {
    object2 (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;
  };
#endif

  #pragma db object
  struct object
  {
    #pragma db id
    unsigned long id_;

#if MODEL_VERSION == 2
    object (unsigned long id = 0): id_ (id) {}
#else
    object1* o1;

    // There is no support for changing a column to NOT NULL in SQLite.
    //
#ifndef ODB_DATABASE_SQLITE
    #pragma db not_null
#endif
    object2* o2;

    object (unsigned long id = 0): id_ (id), o1 (0), o2 (0) {}
    ~object () {delete o1; delete o2;}
#endif
  };
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
