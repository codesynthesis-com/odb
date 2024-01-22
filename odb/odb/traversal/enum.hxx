// file      : odb/traversal/enum.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_ENUM_HXX
#define ODB_TRAVERSAL_ENUM_HXX

#include <odb/semantics/enum.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct enumerates: edge<semantics::enumerates>
  {
    enumerates () {}
    enumerates (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct enumerator: node<semantics::enumerator> {};

  struct underlies: edge<semantics::underlies>
  {
    underlies () {}
    underlies (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct enum_: node<semantics::enum_>
  {
    virtual void
    traverse (type&);

    virtual void
    underlied (type&);

    virtual void
    underlied (type&, edge_dispatcher&);

    virtual void
    enumerates (type&);

    virtual void
    enumerates (type&, edge_dispatcher&);
  };
}

#endif // ODB_TRAVERSAL_ENUM_HXX
