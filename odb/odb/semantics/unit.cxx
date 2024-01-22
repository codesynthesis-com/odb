// file      : odb/semantics/unit.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/unit.hxx>

namespace semantics
{
  unit::
  unit (path const& file)
      : node (file, 1, 1, global_namespace), graph_ (*this)
  {
    // Use a special edge to get this->name() return the global
    // namespace name ("").
    //
    new_edge<global_names> (*this, *this);
    node::unit (*this);
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

        // unit
        //
        {
          type_info ti (typeid (unit));
          ti.add_base (typeid (namespace_));
          insert (ti);
        }
      }
    } init_;
  }
}
