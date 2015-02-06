// file      : evolution/drop-column/model.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
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
  #pragma db value
  struct value
  {
    value (int x_ = 0, int y_ = 0): x (x_), y (y_) {}
    int x;
    int y;
  };

  #pragma db object
  struct object1
  {
    object1 (int x = 0, int y = 0): id (x, y) {}

    #pragma db id
    value id;
  };

  #pragma db object
  struct object
  {
    object (unsigned long id = 0): id_ (id), ptr (0) {}
    ~object () {delete ptr;}

    #pragma db id
    unsigned long id_;

    std::string str;
    unsigned long num;
    object1* ptr;
  };

#if MODEL_VERSION == 3
  #pragma db member(object::str) deleted(3)
  #pragma db member(object::num) deleted(3)
  #pragma db member(object::ptr) deleted(3)
#endif
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
