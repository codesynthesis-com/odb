// file      : evolution/drop-column/model.hxx
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
    object1 (int i = 0): id (i) {}

    #pragma db id
    int id;
  };

  #pragma db object
  struct object2
  {
    object2 (int x = 0, int y = 0): id (x, y) {}

    #pragma db id
    value id;
  };

  #pragma db object
  struct object
  {
    object (unsigned long id = 0): id_ (id), ptr1 (nullptr), ptr2 (nullptr) {}
    ~object () {delete ptr1; delete ptr2;}

    #pragma db id
    unsigned long id_;

    std::string str;
    unsigned long num;
    object1* ptr1;
    object2* ptr2; // Separate FK constraint.
  };

#if MODEL_VERSION == 3
  #pragma db member(object::str) deleted(3)
  #pragma db member(object::num) deleted(3)
  #pragma db member(object::ptr1) deleted(3)
  #pragma db member(object::ptr2) deleted(3)
#endif
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
