// file      : odb/traversal/class.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_CLASS_HXX
#define ODB_TRAVERSAL_CLASS_HXX

#include <odb/semantics/class.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct inherits: edge<semantics::inherits>
  {
    inherits ()
    {
    }

    inherits (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct class_: scope_template<semantics::class_>
  {
    virtual void
    traverse (type&);

    virtual void
    inherits (type&);

    virtual void
    inherits (type&, edge_dispatcher&);
  };
}

#endif // ODB_TRAVERSAL_CLASS_HXX
