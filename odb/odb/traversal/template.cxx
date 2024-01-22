// file      : odb/traversal/template.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/template.hxx>

namespace traversal
{
  void instantiates::
  traverse (type& i)
  {
    dispatch (i.template_ ());
  }

  //
  //
  void instantiation::
  traverse (type& i)
  {
    instantiates (i);
  }

  void instantiation::
  instantiates (type& i)
  {
    instantiates (i, *this);
  }

  void instantiation::
  instantiates (type& i, edge_dispatcher& d)
  {
    d.dispatch (i.instantiates ());
  }

  //
  //
  void type_instantiation::
  traverse (type& i)
  {
    instantiates (i);
  }

  void type_instantiation::
  instantiates (type& i)
  {
    instantiates (i, *this);
  }

  void type_instantiation::
  instantiates (type& i, edge_dispatcher& d)
  {
    d.dispatch (i.instantiates ());
  }
}
