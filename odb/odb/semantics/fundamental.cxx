// file      : odb/semantics/fundamental.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/fundamental.hxx>

namespace semantics
{
  string fund_type::
  fq_name () const
  {
    return name ();
  }

  string fund_type::
  fq_name (names* hint) const
  {
    if (hint == 0)
      return name ();

    return type::fq_name (hint);
  }

  // char
  //
  bool fund_char::
  unsigned_ () const
  {
    return TYPE_UNSIGNED (tree_node ()) != 0;
  }

  // wchar_t
  //
  bool fund_wchar::
  unsigned_ () const
  {
    return TYPE_UNSIGNED (tree_node ()) != 0;
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

        // fund_type
        //
        {
          type_info ti (typeid (fund_type));
          ti.add_base (typeid (type));
          insert (ti);
        }

        // fund_void
        //
        {
          type_info ti (typeid (fund_void));
          ti.add_base (typeid (fund_type));
          insert (ti);
        }

        // integral_type
        //
        {
          type_info ti (typeid (integral_type));
          ti.add_base (typeid (fund_type));
          insert (ti);
        }

        // fund_bool
        //
        {
          type_info ti (typeid (fund_bool));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_char
        //
        {
          type_info ti (typeid (fund_char));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_wchar
        //
        {
          type_info ti (typeid (fund_wchar));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_char16
        //
        {
          type_info ti (typeid (fund_char16));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_char32
        //
        {
          type_info ti (typeid (fund_char32));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_signed_char
        //
        {
          type_info ti (typeid (fund_signed_char));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_unsigned_char
        //
        {
          type_info ti (typeid (fund_unsigned_char));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_short
        //
        {
          type_info ti (typeid (fund_short));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_unsigned_short
        //
        {
          type_info ti (typeid (fund_unsigned_short));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_int
        //
        {
          type_info ti (typeid (fund_int));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_unsigned_int
        //
        {
          type_info ti (typeid (fund_unsigned_int));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_long
        //
        {
          type_info ti (typeid (fund_long));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_unsigned_long
        //
        {
          type_info ti (typeid (fund_unsigned_long));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_long_long
        //
        {
          type_info ti (typeid (fund_long_long));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_unsigned_long_long
        //
        {
          type_info ti (typeid (fund_unsigned_long_long));
          ti.add_base (typeid (integral_type));
          insert (ti);
        }

        // fund_float
        //
        {
          type_info ti (typeid (fund_float));
          ti.add_base (typeid (fund_type));
          insert (ti);
        }

        // fund_double
        //
        {
          type_info ti (typeid (fund_double));
          ti.add_base (typeid (fund_type));
          insert (ti);
        }

        // fund_long_double
        //
        {
          type_info ti (typeid (fund_long_double));
          ti.add_base (typeid (fund_type));
          insert (ti);
        }
      }
    } init_;
  }
}
