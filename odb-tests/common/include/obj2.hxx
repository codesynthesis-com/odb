// file      : common/include/obj2.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef OBJ2_HXX
#define OBJ2_HXX

#include <odb/core.hxx>

#pragma db object
struct object2
{
  object2 (unsigned long id)
      : id_ (id)
  {
  }

  object2 ()
  {
  }

  #pragma db id
  unsigned long id_;
};

#endif // OBJ2_HXX
