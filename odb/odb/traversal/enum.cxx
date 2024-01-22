// file      : odb/traversal/enum.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/traversal/enum.hxx>

namespace traversal
{
  void enumerates::
  traverse (type& e)
  {
    dispatch (e.enumerator ());
  }

  //
  //
  void underlies::
  traverse (type& u)
  {
    dispatch (u.type ());
  }

  //
  //
  void enum_::
  traverse (type& e)
  {
    underlied (e);
    enumerates (e);
  }

  void enum_::
  underlied (type& e)
  {
    underlied (e, *this);
  }

  void enum_::
  underlied (type& e, edge_dispatcher& d)
  {
    d.dispatch (e.underlied ());
  }

  void enum_::
  enumerates (type& e)
  {
    enumerates (e, *this);
  }

  void enum_::
  enumerates (type& e, edge_dispatcher& d)
  {
    iterate_and_dispatch (e.enumerates_begin (), e.enumerates_end (), d);
  }
}
