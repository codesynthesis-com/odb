// file      : odb/traversal/relational/column.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_COLUMN_HXX
#define ODB_TRAVERSAL_RELATIONAL_COLUMN_HXX

#include <odb/semantics/relational/column.hxx>
#include <odb/traversal/relational/elements.hxx>

namespace traversal
{
  namespace relational
  {
    struct column: node<semantics::relational::column> {};
    struct add_column: node<semantics::relational::add_column> {};
    struct drop_column: node<semantics::relational::drop_column> {};
    struct alter_column: node<semantics::relational::alter_column> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_COLUMN_HXX
