// file      : odb/traversal/class-template.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_CLASS_TEMPLATE_HXX
#define ODB_TRAVERSAL_CLASS_TEMPLATE_HXX

#include <odb/semantics/class-template.hxx>

#include <odb/traversal/elements.hxx>
#include <odb/traversal/class.hxx>

namespace traversal
{
  struct class_template: scope_template<semantics::class_template>
  {
    virtual void
    traverse (type&);

    virtual void
    inherits (type&);

    virtual void
    inherits (type&, edge_dispatcher&);
  };

  struct class_instantiation: scope_template<semantics::class_instantiation>
  {
    virtual void
    traverse (type&);

    virtual void
    instantiates (type&);

    virtual void
    instantiates (type&, edge_dispatcher&);

    virtual void
    inherits (type&);

    virtual void
    inherits (type&, edge_dispatcher&);
  };
}

#endif // ODB_TRAVERSAL_CLASS_TEMPLATE_HXX
