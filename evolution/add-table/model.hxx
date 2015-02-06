// file      : evolution/add-table/model.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <vector>
#include <string>

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

    std::string str;
  };

#if MODEL_VERSION == 3
  #pragma db object
  struct object1
  {
    object1 (): o (0) {}
    ~object1 () {delete o;}

    #pragma db id auto
    unsigned long id_;

    object* o;
    std::vector<int> nums;
  };
#endif
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
