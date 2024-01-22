// file      : odb/semantics/union-template.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_UNION_TEMPLATE_HXX
#define ODB_SEMANTICS_UNION_TEMPLATE_HXX

#include <odb/semantics/elements.hxx>
#include <odb/semantics/union.hxx>
#include <odb/semantics/template.hxx>

namespace semantics
{
  class union_template: public type_template, public scope
  {
  public:
    union_template (path const&, size_t line, size_t column, tree);

    // Resolve conflict between scope::scope and nameable::scope.
    //
    using nameable::scope;
  };

  class union_instantiation: public union_, public type_instantiation
  {
  public:
    union_instantiation (path const&, size_t line, size_t column, tree);

    using union_::add_edge_left;
    using type_instantiation::add_edge_left;
  };
}

#endif // ODB_SEMANTICS_UNION_TEMPLATE_HXX
