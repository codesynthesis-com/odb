// file      : odb/semantics/class-template.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/class-template.hxx>

namespace semantics
{
  class_template::
  class_template (path const& file, size_t line, size_t column, tree tn)
      : node (file, line, column, tn)
  {
  }

  class_instantiation::
  class_instantiation (path const& file,
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

        // class_template
        //
        {
          type_info ti (typeid (class_template));
          ti.add_base (typeid (type_template));
          ti.add_base (typeid (scope));
          insert (ti);
        }

        // class_instantiation
        //
        {
          type_info ti (typeid (class_instantiation));
          ti.add_base (typeid (class_));
          ti.add_base (typeid (type_instantiation));
          insert (ti);
        }
      }
    } init_;
  }
}
