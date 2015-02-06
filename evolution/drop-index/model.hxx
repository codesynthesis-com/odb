// file      : evolution/drop-index/model.hxx
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
  #pragma db object
  struct object
  {
    object (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;

#if MODEL_VERSION == 2
    #pragma db unique
#endif
    unsigned long num;
  };
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
