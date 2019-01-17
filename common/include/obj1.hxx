// file      : common/include/obj1.hxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef OBJ1_HXX
#define OBJ1_HXX

#include <odb/core.hxx>

#pragma db object
struct object1
{
  object1 (unsigned long id)
      : id_ (id)
  {
  }

  object1 ()
  {
  }

  #pragma db id
  unsigned long id_;
};

#endif // OBJ1_HXX
