// file      : odb/traversal/class-template.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/class-template.hxx>

namespace traversal
{
  //
  //
  void class_template::
  traverse (type& c)
  {
    inherits (c);
    names (c);
  }

  void class_template::
  inherits (type& c)
  {
    inherits (c, *this);
  }

  void class_template::
  inherits (type& c, edge_dispatcher& d)
  {
    iterate_and_dispatch (c.inherits_begin (), c.inherits_end (), d);
  }

  //
  //
  void class_instantiation::
  traverse (type& c)
  {
    instantiates (c);
    inherits (c);
    names (c);
  }

  void class_instantiation::
  instantiates (type& c)
  {
    instantiates (c, *this);
  }

  void class_instantiation::
  instantiates (type& c, edge_dispatcher& d)
  {
    d.dispatch (c.instantiates ());
  }

  void class_instantiation::
  inherits (type& c)
  {
    inherits (c, *this);
  }

  void class_instantiation::
  inherits (type& c, edge_dispatcher& d)
  {
    iterate_and_dispatch (c.inherits_begin (), c.inherits_end (), d);
  }
}
