// file      : tracer/include/obj3.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef OBJ3_HXX
#define OBJ3_HXX

#include <odb/core.hxx>

#pragma db object
struct object3
{
  object3 (unsigned long id)
      : id_ (id)
  {
  }

  object3 ()
  {
  }

  #pragma db id
  unsigned long id_;
};

#endif // OBJ3_HXX
