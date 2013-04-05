// file      : evolution/alter-column/model.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

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

#if MODEL_VERSION == 2
    odb::nullable<std::string> str;

    unsigned long num;

    #pragma db null
    unsigned long dummy; // Test multiple ALTER COLUMN clauses.
#else
    // Use nullable to be able to access during migration.
    //
    #pragma db not_null
    odb::nullable<std::string> str;

    odb::nullable<unsigned long> num;

    unsigned long dummy;
#endif
  };
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
