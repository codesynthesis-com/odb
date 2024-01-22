// file      : odb/semantics/enum.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/enum.hxx>

namespace semantics
{
  enumerates::
  enumerates ()
  {
  }

  enumerator::
  enumerator (path const& file,
              size_t line,
              size_t column,
              tree tn,
              unsigned long long value)
      : node (file, line, column, tn), value_ (value)
  {
  }

  underlies::
  underlies ()
      : type_ (0), enum__ (0), hint_ (0)
  {
  }

  enum_::
  enum_ (path const& file,
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

        // enumerates
        //
        {
          type_info ti (typeid (enumerates));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // enumerator
        //
        {
          type_info ti (typeid (enumerator));
          ti.add_base (typeid (nameable));
          ti.add_base (typeid (instance));
          insert (ti);
        }

        // underlies
        //
        {
          type_info ti (typeid (underlies));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // enum_
        //
        {
          type_info ti (typeid (enum_));
          ti.add_base (typeid (type));
          ti.add_base (typeid (scope));
          insert (ti);
        }
      }
    } init_;
  }
}
