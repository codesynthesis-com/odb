// file      : odb/traversal/relational/foreign-key.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_RELATIONAL_FOREIGN_KEY_HXX
#define ODB_TRAVERSAL_RELATIONAL_FOREIGN_KEY_HXX

#include <odb/semantics/relational/foreign-key.hxx>
#include <odb/traversal/relational/key.hxx>

namespace traversal
{
  namespace relational
  {
    struct foreign_key: key_template<semantics::relational::foreign_key> {};
    struct add_foreign_key:
      key_template<semantics::relational::add_foreign_key> {};
    struct drop_foreign_key: node<semantics::relational::drop_foreign_key> {};
  }
}

#endif // ODB_TRAVERSAL_RELATIONAL_FOREIGN_KEY_HXX
