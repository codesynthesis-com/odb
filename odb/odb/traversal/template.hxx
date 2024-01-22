// file      : odb/traversal/template.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_TEMPLATE_HXX
#define ODB_TRAVERSAL_TEMPLATE_HXX

#include <odb/semantics/template.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct instantiates: edge<semantics::instantiates>
  {
    instantiates ()
    {
    }

    instantiates (node_dispatcher& n)
    {
      node_traverser (n);
    }

    virtual void
    traverse (type&);
  };

  struct template_: node<semantics::template_> {};

  struct instantiation: node<semantics::instantiation>
  {
    virtual void
    traverse (type&);

    virtual void
    instantiates (type&);

    virtual void
    instantiates (type&, edge_dispatcher&);
  };

  struct type_template: node<semantics::type_template> {};

  struct type_instantiation: node<semantics::type_instantiation>
  {
    virtual void
    traverse (type&);

    virtual void
    instantiates (type&);

    virtual void
    instantiates (type&, edge_dispatcher&);
  };
}

#endif // ODB_TRAVERSAL_TEMPLATE_HXX
