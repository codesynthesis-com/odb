// file      : evolution/drop-table/model.hxx
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

#if MODEL_VERSION == 3
  #pragma db object(object1) deleted(3)
#endif

  // Make sure we also clean up base tables when dropping a
  // table corresponding to the polymorphic derived object.
  //
  #pragma db value
  struct value
  {
    value (unsigned long n = 0, const std::string& s = ""): num (n), str (s) {}

    unsigned long num;
    std::string str;
  };

  #pragma db object polymorphic
  struct root
  {
    root (unsigned long n = 0, const std::string& s = ""): id (n, s) {}
    virtual ~root () {}

    #pragma db id
    value id;
  };

  #pragma db object
  struct base: root
  {
    base (unsigned long n = 0, const std::string& s = "")
        : root (n, s), num (n) {}

    unsigned long num;
  };

  #pragma db object
  struct derived: base
  {
    derived (unsigned long n = 0, const std::string& s = "")
        : base (n, s), str (s) {}

    std::string str;
  };

#if MODEL_VERSION == 3
  #pragma db object(derived) deleted(3)
#endif
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
