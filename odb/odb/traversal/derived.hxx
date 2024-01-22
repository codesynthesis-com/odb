// file      : odb/traversal/derived.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_DERIVED_HXX
#define ODB_TRAVERSAL_DERIVED_HXX

#include <odb/semantics/derived.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  //
  // Edges.
  //
  struct qualifies: edge<semantics::qualifies>
  {
    qualifies ()
    {
    }

    qualifies (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct points: edge<semantics::points>
  {
    points ()
    {
    }

    points (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct references: edge<semantics::references>
  {
    references ()
    {
    }

    references (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct contains: edge<semantics::contains>
  {
    contains ()
    {
    }

    contains (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };


  //
  // Nodes.
  //
  struct derived_type: node<semantics::derived_type> {};

  struct qualifier: node<semantics::qualifier>
  {
    virtual void
    traverse (type&);

    virtual void
    qualifies (type&);

    virtual void
    qualifies (type&, edge_dispatcher&);
  };

  struct pointer: node<semantics::pointer>
  {
    virtual void
    traverse (type&);

    virtual void
    points (type&);

    virtual void
    points (type&, edge_dispatcher&);
  };

  struct reference: node<semantics::reference>
  {
    virtual void
    traverse (type&);

    virtual void
    references (type&);

    virtual void
    references (type&, edge_dispatcher&);
  };

  struct array: node<semantics::array>
  {
    virtual void
    traverse (type&);

    virtual void
    contains (type&);

    virtual void
    contains (type&, edge_dispatcher&);
  };
}

#endif // ODB_TRAVERSAL_DERIVED_HXX
