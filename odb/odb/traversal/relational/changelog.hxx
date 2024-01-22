// file      : odb/traversal/relational/changelog.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_CHANGELOG_HXX
#define ODB_TRAVERSAL_RELATIONAL_CHANGELOG_HXX

#include <odb/semantics/relational/changelog.hxx>
#include <odb/traversal/relational/elements.hxx>

namespace traversal
{
  namespace relational
  {
    struct contains_model: edge<semantics::relational::contains_model>
    {
      contains_model () {}
      contains_model (node_dispatcher& n) {node_traverser (n);}

      virtual void
      traverse (type&);
    };

    struct contains_changeset: edge<semantics::relational::contains_changeset>
    {
      contains_changeset () {}
      contains_changeset (node_dispatcher& n) {node_traverser (n);}

      virtual void
      traverse (type&);
    };

    struct changelog: node<semantics::relational::changelog>
    {
    public:
      virtual void
      traverse (type&);

      virtual void
      contains_model (type&);

      virtual void
      contains_model (type&, edge_dispatcher&);

      virtual void
      contains_changeset (type&);

      virtual void
      contains_changeset (type&, edge_dispatcher&);
    };
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_CHANGELOG_HXX
