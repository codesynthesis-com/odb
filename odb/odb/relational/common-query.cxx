// file      : odb/relational/common-query.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/diagnostics.hxx>

#include <odb/relational/common-query.hxx>

using namespace std;

namespace relational
{
  // query_alias_traits
  //

  void query_alias_traits::
  generate_decl_body ()
  {
    os << "static const char table_name[];";
  }

  void query_alias_traits::
  generate_def (semantics::data_member& m, semantics::class_& c)
  {
    // Come up with a table alias. Generally, we want it to be based
    // on the column name. This is straightforward for single-column
    // references. In case of a composite id, we will need to use the
    // column prefix which is based on the data member name, unless
    // overridden by the user. In the latter case the prefix can be
    // empty, in which case we will just fall back on the member's
    // public name.
    //
    string alias;
    {
      string n;

      if (composite_wrapper (utype (*id_member (c))))
      {
        n = column_prefix (m, key_prefix_, default_name_).prefix;

        if (n.empty ())
          n = public_name_db (m);
        else if (n[n.size () - 1] == '_')
          n.resize (n.size () - 1); // Remove trailing underscore.
      }
      else
      {
        bool dummy;
        n = column_name (m, key_prefix_, default_name_, dummy);
      }

      alias = column_prefix_.prefix + n;
    }

    generate_def (public_name (m), c, alias);
  }

  void query_alias_traits::
  generate_def (string const& tag, semantics::class_& c, string const& alias)
  {
    semantics::class_* poly_root (polymorphic (c));
    bool poly_derived (poly_root != 0 && poly_root != &c);
    semantics::class_* poly_base (poly_derived ? &polymorphic_base (c) : 0);

    if (poly_derived)
      generate_def (tag, *poly_base, alias);

    os << "const char alias_traits<"
       << "  " << class_fq_name (c) << "," << endl
       << "  id_" << db << "," << endl
       << "  " << scope_ << "::" << tag << "_tag>::" << endl
       << "table_name[] = ";

    if (poly_root != 0)
      os << strlit (quote_id (alias + "_" + table_name (c).uname ()));
    else
      os << strlit (quote_id (alias));

    os << ";"
       << endl;
  }

  entry<query_alias_traits> query_alias_traits_;


  // query_columns_base
  //

  entry<query_columns_base> query_columns_base_;

  // query_columns
  //

  void query_columns::
  column_ctor (string const& type, string const& name, string const& base)
  {
    os << name << " (";

    if (multi_dynamic)
      os << "odb::query_column< " << type
         << ", odb::default_query_column_base< " << type << " > >& qc," << endl;

    os << "const char* t, const char* c, const char* conv)" << endl
       << "  : " << base << " (" << (multi_dynamic ? "qc, " : "") <<
      "t, c, conv)"
       << "{"
       << "}";
  }

  void query_columns::
  column_ctor_args_extra (semantics::data_member&)
  {
  }

  void query_columns::
  column_common (semantics::data_member& m,
                 string const& type,
                 string const& column,
                 const custom_cxx_type* ct,
                 string const& suffix)
  {
    string name (public_name (m));

    if (decl_)
    {
      string type_id (database_type_id (m));

      os << "// " << name << endl
         << "//" << endl;

      if (ct == 0)
      {
        os << "typedef" << endl
           << db << "::query_column<" << endl
           << "  " << db << "::value_traits<" << endl
           << "    " << type << "," << endl
           << "    " << type_id << " >::query_type," << endl
           << "  " << type_id << "," << endl
           << "  " << db << "::default_query_column_base<" << endl
           << "    " << type << "," << endl
           << "    " << type_id << " > >" << endl
           << name << suffix << ";"
           << endl;
      }
      else
      {
        string mapped_type (ct->type->fq_name (ct->type_hint));
        string base_type (name + "_base" + suffix); // query_column<> base type.

        // Base type for query_column<>.
        //
        os << "struct " << base_type << ": " << db << "::query_column_base"
           << "{"
           << "using query_column_base::query_column_base;" << endl
           << "static void" << endl
           << "append (" << db << "::query_base& q," << endl
           << db << "::val_bind<" << mapped_type << "> v," << endl
           << "const char* conv)"
           << "{"
           << "// From " << location_string (ct->loc, true) << endl
           << type_ref_type (*ct->as, ct->as_hint, true, "vt") << " =" << endl
           << "  " << ct->translate_to ("v.val") << ";" << endl
           << "q.append<" << endl
           << "  " << type << "," << endl
           << "  " << type_id << "> (" << endl
           << "  " << db << "::val_bind<" << type << "> (vt)," << endl
           << "  conv);"
           << "}";


        // @@ TODO Check if the 'to' clause expression is of a reference type
        //         and, if that's the case, generate a proper function instead.
        //
        // @@ Can we fix ODB to add '\n' automatically after this function
        //    declaration, as we do for the above function definition?
        //
        os << "// If a compiler error points to the line below, then it most likely" << endl
           << "// means that the 'to' clause expression of the respective map pragma is" << endl
           << "// not of a reference type and thus binding by reference is not" << endl
           << "// supported for this query member." << endl
           << "//" << endl
           << "static void" << endl
           << "append (" << db << "::query_base&," << endl
           << db << "::ref_bind<" << mapped_type << ">," << endl
           << "const char*) = delete;" << endl;

        if (multi_dynamic)
        {
          os << "static void*" << endl
             << "param_factory ()"
             << "{"
             << db << "::query_param_factory f (" << endl
             << "&" << db << "::query_param_factory_impl<" << endl
             << "  " << type << "," << endl
             << "  " << type_id << ">);" << endl
             << "return reinterpret_cast<void*> (f);"
             << "}";
        }
        else
        {
          // @@ TODO: Write comment.
          //
          os << "static void*" << endl
             << "param_factory () = delete;";
        }

        os << "};";

        // Type for query_columns<> member.
        //
        os << "typedef" << endl
           << db << "::query_column<" << endl
           << "  " << mapped_type << "," << endl
           << "  " << type_id << "," << endl
           << "  " << base_type << " >" << endl
           << name << suffix << ";"
           << endl;
      }
    }
    else
    {
      // Note that here we don't use suffix.
      //
      string tmpl (ptr_ ? "pointer_query_columns" : "query_columns");
      tmpl += "< " + fq_name_ + ", id_" + db.string () + ", A >" + scope_;

      os << "template <typename A>" << endl
         << "const typename " << tmpl << "::" << name << "_type_" << endl
         << tmpl << "::" << endl
         << name << " (";

      // Pass common query column for registration.
      //
      if (multi_dynamic)
      {
        string tmpl (ptr_ ? "pointer_query_columns" : "query_columns");
        tmpl += "< " + fq_name_ + ", id_common, typename A::common_traits >" +
          scope_;

        os << tmpl << "::" << name << "," << endl;
      }

      os << "A::" << "table_name, " << strlit (quote_id (column));

      string const& conv (convert_to_expr (column_type (), m));
      os << ", " << (conv.empty () ? "0" : strlit (conv));

      column_ctor_args_extra (m);

      os << ");"
         << endl;
    }
  }
}
