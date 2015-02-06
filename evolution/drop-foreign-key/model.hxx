// file      : evolution/drop-foreign-key/model.hxx
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
  #pragma db object
  struct object
  {
    #pragma db id
    unsigned long id_;

    // SQLite doesn't support dropping of foreign keys.
    //
#ifndef DATABASE_SQLITE
#if MODEL_VERSION == 2
    object* o1;
    object* o2;

    object (unsigned long id = 0): id_ (id), o1 (0), o2 (0) {}
    ~object () {delete o1; delete o2;}
#else
    #pragma db null
    unsigned long o1;

    #pragma db null
    unsigned long o2;

    object (unsigned long id = 0): id_ (id) {}
#endif
#endif
  };
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
