// file      : odb/traversal/relational/changeset.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_CHANGESET_HXX
#define ODB_TRAVERSAL_RELATIONAL_CHANGESET_HXX

#include <odb/semantics/relational/changeset.hxx>
#include <odb/traversal/relational/elements.hxx>

namespace traversal
{
  namespace relational
  {
    struct changeset: scope_template<semantics::relational::changeset> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_CHANGESET_HXX
