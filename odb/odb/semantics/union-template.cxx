// file      : odb/semantics/union-template.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/union-template.hxx>

namespace semantics
{
  union_template::
  union_template (path const& file, size_t line, size_t column, tree tn)
      : node (file, line, column, tn)
  {
  }

  union_instantiation::
  union_instantiation (path const& file,
                       size_t line,
                       size_t column,
                       tree tn)
      : node (file, line, column, tn)
  {
  }

  // type info
  //
  namespace
  {
    struct init
    {
      init ()
      {
        using compiler::type_info;

        // union_template
        //
        {
          type_info ti (typeid (union_template));
          ti.add_base (typeid (type_template));
          ti.add_base (typeid (scope));
          insert (ti);
        }

        // union_instantiation
        //
        {
          type_info ti (typeid (union_instantiation));
          ti.add_base (typeid (union_));
          ti.add_base (typeid (type_instantiation));
          insert (ti);
        }
      }
    } init_;
  }
}
