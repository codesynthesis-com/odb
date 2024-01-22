// file      : odb/traversal/union-template.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/union-template.hxx>

namespace traversal
{
  void union_instantiation::
  traverse (type& u)
  {
    instantiates (u);
    names (u);
  }

  void union_instantiation::
  instantiates (type& u)
  {
    instantiates (u, *this);
  }

  void union_instantiation::
  instantiates (type& u, edge_dispatcher& d)
  {
    d.dispatch (u.instantiates ());
  }
}
