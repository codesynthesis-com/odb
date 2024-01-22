// file      : odb/traversal/relational/table.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_TABLE_HXX
#define ODB_TRAVERSAL_RELATIONAL_TABLE_HXX

#include <odb/semantics/relational/table.hxx>
#include <odb/traversal/relational/elements.hxx>

namespace traversal
{
  namespace relational
  {
    struct table: scope_template<semantics::relational::table> {};
    struct add_table: scope_template<semantics::relational::add_table> {};
    struct drop_table: node<semantics::relational::drop_table> {};
    struct alter_table: scope_template<semantics::relational::alter_table> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_TABLE_HXX
