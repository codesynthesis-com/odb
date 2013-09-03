// file      : evolution/drop-column/model.hxx
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
  #pragma db object
  struct object
  {
    object (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;

    // SQLite doesn't support dropping of columns.
    //
#ifndef DATABASE_SQLITE
#if MODEL_VERSION >= 2

#if MODEL_VERSION == 3
    #pragma db deleted(3)
#endif
    std::string str;

#if MODEL_VERSION == 3
    #pragma db deleted(3)
#endif
    unsigned long num;

#endif
#endif
  };
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
