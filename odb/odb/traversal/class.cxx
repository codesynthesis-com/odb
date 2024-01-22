// file      : odb/traversal/class.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/class.hxx>

namespace traversal
{
  void inherits::
  traverse (type& i)
  {
    dispatch (i.base ());
  }

  //
  //
  void class_::
  traverse (type& c)
  {
    inherits (c);
    names (c);
  }

  void class_::
  inherits (type& c)
  {
    inherits (c, *this);
  }

  void class_::
  inherits (type& c, edge_dispatcher& d)
  {
    iterate_and_dispatch (c.inherits_begin (), c.inherits_end (), d);
  }
}
