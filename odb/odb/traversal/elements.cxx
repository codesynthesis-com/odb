// file      : odb/traversal/elements.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/elements.hxx>

namespace traversal
{
  void names::
  traverse (type& e)
  {
    dispatch (e.named ());
  }

  void declares::
  traverse (type& e)
  {
    dispatch (e.named ());
  }

  void defines::
  traverse (type& e)
  {
    dispatch (e.named ());
  }

  void typedefs::
  traverse (type& e)
  {
    dispatch (e.named ());
  }

  void belongs::
  traverse (type& e)
  {
    dispatch (e.type ());
  }

  // instance
  //
  void instance::
  traverse (type& i)
  {
    belongs (i);
  }

  void instance::
  belongs (type& i)
  {
    belongs (i, *this);
  }

  void instance::
  belongs (type& i, edge_dispatcher& d)
  {
    d.dispatch (i.belongs ());
  }

  // data_member
  //
  void data_member::
  traverse (type& m)
  {
    belongs (m);
  }

  void data_member::
  belongs (type& m)
  {
    belongs (m, *this);
  }

  void data_member::
  belongs (type& m, edge_dispatcher& d)
  {
    d.dispatch (m.belongs ());
  }
}
