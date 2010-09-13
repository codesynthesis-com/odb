// file      : tracer/template/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>

#pragma db value(bool) type ("INT")

struct x {};
#pragma db value (x)

namespace N
{
  #pragma db object
  struct object1
  {
    object1 () {}

    #pragma db id
    unsigned long id_;

    #pragma db member type ("INT")
    bool b_;
  };

  struct object2
  {
    object2 () {}

    unsigned long id_;
  };

  #pragma db object (object2)
}

PRAGMA_DB (member (N::object2::id_) id auto);

#endif // TEST_HXX
