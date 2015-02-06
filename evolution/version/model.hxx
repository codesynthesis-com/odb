// file      : evolution/version/model.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <odb/core.hxx>

#pragma db model version(1, MODEL_VERSION)

#define MODEL_NAMESPACE_IMPL(V) v##V
#define MODEL_NAMESPACE(V) MODEL_NAMESPACE_IMPL(V)

namespace MODEL_NAMESPACE(MODEL_VERSION)
{
#if MODEL_VERSION == 2
  #pragma db object
  struct object1
  {
    object1 (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;

    int num;
  };
#endif

#if MODEL_VERSION == 3
  #pragma db object
  struct object2
  {
    object2 (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;

    int num;
  };
#endif
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
