// file      : odb/semantics/union.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/union.hxx>

namespace semantics
{
  union_::
  union_ (path const& file, size_t line, size_t column, tree tn)
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

        // union_
        //
        {
          type_info ti (typeid (union_));
          ti.add_base (typeid (type));
          ti.add_base (typeid (scope));
          insert (ti);
        }
      }
    } init_;
  }
}
