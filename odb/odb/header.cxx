// file      : odb/header.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/common.hxx>
#include <odb/context.hxx>
#include <odb/generate.hxx>

using namespace std;

namespace header
{
  struct class1: traversal::class_, virtual context
  {
    class1 ()
        : typedefs_ (false),
          query_columns_type_ (false, true, false),
          pointer_query_columns_type_ (true, true, false)
    {
      *this >> defines_ >> *this;
      *this >> typedefs_ >> *this;
    }

    virtual void
    traverse (type& c)
    {
      class_kind_type ck (class_kind (c));

      if (ck == class_other ||
          (!options.at_once () && class_file (c) != unit.file ()))
        return;

      names (c);

      switch (ck)
      {
      case class_object: traverse_object (c); break;
      case class_view: traverse_view (c); break;
      default: break;
      }
    }

    void
    traverse_object (type&);

    void
    traverse_view (type&);

  private:
    traversal::defines defines_;
    typedefs typedefs_;

    instance<query_columns_type> query_columns_type_;
    instance<query_columns_type> pointer_query_columns_type_;
  };
}

void header::class1::
traverse_object (type& c)
{
  using semantics::data_member;

  data_member_path* id (id_member (c));
  data_member* idf (id ? id->front () : 0);
  bool auto_id (id && auto_ (*id));
  bool base_id (id && &idf->scope () != &c); // Comes from base.

  data_member* opt (context::optimistic (c));

  type* poly_root (polymorphic (c));
  bool poly (poly_root != 0);
  bool poly_derived (poly && poly_root != &c);
  type* poly_base (poly_derived ? &polymorphic_base (c) : 0);
  data_member* discriminator (poly ? context::discriminator (*poly_root) : 0);

  bool abst (abstract (c));
  bool reuse_abst (abst && !poly);

  user_sections& uss (c.get<user_sections> ("user-sections"));

  string const& type (class_fq_name (c));

  os << "// " << class_name (c) << endl
     << "//" << endl;

  // class_traits
  //
  os << "template <>" << endl
     << "struct class_traits< " << type << " >"
     << "{"
     << "static const class_kind kind = class_object;"
     << "};";

  // object_traits
  //
  os << "template <>" << endl
     << "class " << exp << "access::object_traits< " << type << " >"
     << "{"
     << "public:" << endl;

  // object_type & pointer_type
  //
  os << "typedef " << type << " object_type;"
     << "typedef " << c.get<string> ("object-pointer") << " pointer_type;"
     << "typedef odb::pointer_traits<pointer_type> pointer_traits;"
     << endl;

  // polymorphic, root_type, base_type, etc.
  //
  os << "static const bool polymorphic = " << poly << ";"
     << endl;

  if (poly)
  {
    os << "typedef " << class_fq_name (*poly_root) << " root_type;";

    if (poly_derived)
    {
      os << "typedef " << class_fq_name (*poly_base) << " base_type;"
         << "typedef object_traits<root_type>::discriminator_type " <<
        "discriminator_type;"
         << "typedef polymorphic_concrete_info<root_type> info_type;";

      if (abst)
        os << "typedef polymorphic_abstract_info<root_type> " <<
          "abstract_info_type;";

      // Calculate our hierarchy depth (number of classes).
      //
      size_t depth (polymorphic_depth (c));

      os << endl
         << "static const std::size_t depth = " << depth << "UL;";
    }
    else
    {
      semantics::names* hint;
      semantics::type& t (utype (*discriminator, hint));

      os << "typedef " << t.fq_name (hint) << " discriminator_type;"
         << "typedef polymorphic_map<object_type> map_type;"
         << "typedef polymorphic_concrete_info<object_type> info_type;";

      if (abst)
        os << "typedef polymorphic_abstract_info<object_type> " <<
          "abstract_info_type;";

      os << endl
         << "static const std::size_t depth = 1UL;";
    }

    os << endl;
  }

  // id_type, version_type, etc.
  //
  if (id != 0)
  {
    if (base_id)
    {
      semantics::class_& b (dynamic_cast<semantics::class_&> (idf->scope ()));
      string const& type (class_fq_name (b));

      os << "typedef object_traits< " << type << " >::id_type id_type;";

      if (opt != 0)
        os << "typedef object_traits< " << type << " >::version_type " <<
          "version_type;";

      os << endl;

      if (poly_derived)
        os << "static const bool auto_id = false;";
      else
        os << "static const bool auto_id = object_traits< " << type <<
          " >::auto_id;";
    }
    else
    {
      {
        semantics::names* hint;
        semantics::type& t (utype (*id, hint));
        os << "typedef " << t.fq_name (hint) << " id_type;";
      }

      if (opt != 0)
      {
        semantics::names* hint;
        semantics::type& t (utype (*opt, hint));
        os << "typedef " << t.fq_name (hint) << " version_type;";
      }

      os << endl
         << "static const bool auto_id = " << auto_id << ";";
    }

    os << endl;
  }
  else if (!reuse_abst)
  {
    // Object without id.
    //
    os << "typedef void id_type;"
       << endl
       << "static const bool auto_id = false;"
       << endl;
  }

  // abstract
  //
  os << "static const bool abstract = " << abst << ";"
     << endl;

  // id()
  //
  if (id != 0 || !reuse_abst)
  {
    // We want to generate a dummy void id() accessor even if this
    // object has no id to help us in the runtime. This way we can
    // write generic code that will work for both void and non-void
    // ids.
    //
    os << "static id_type" << endl
       << "id (const object_type&);"
       << endl;
  }

  // version()
  //
  if (opt != 0)
  {
    os << "static version_type" << endl
       << "version (const object_type&);"
       << endl;
  }

  // Query.
  //
  if (options.generate_query ())
  {
    // Generate object pointer tags here if we are generating dynamic
    // multi-database support.
    //
    if (multi_dynamic && has_a (c, test_pointer | exclude_base))
    {
      query_tags t;
      t.traverse (c);
    }
  }

  // The rest does not apply to reuse-abstract objects.
  //
  if (!reuse_abst)
  {
    // Cache traits typedefs.
    //
    if (id == 0)
    {
      os << "typedef" << endl
         << "no_id_pointer_cache_traits<pointer_type>" << endl
         << "pointer_cache_traits;"
         << endl
         << "typedef" << endl
         << "no_id_reference_cache_traits<object_type>" << endl
         << "reference_cache_traits;"
         << endl;
    }
    else
    {
      char const* obj (poly_derived ? "root_type" : "object_type");
      char const* ptr (poly_derived
                       ? "object_traits<root_type>::pointer_type"
                       : "pointer_type");
      if (session (c))
      {
        string const& s (options.session_type ());

        os << "typedef" << endl
           << "odb::pointer_cache_traits<" << endl
           << "  " << ptr << "," << endl
           << "  " << s << " >" << endl
           << "pointer_cache_traits;"
           << endl
           << "typedef" << endl
           << "odb::reference_cache_traits<" << endl
           << "  " << obj << "," << endl
           << "  " << s << " >" << endl
           << "reference_cache_traits;"
           << endl;
      }
      else
      {
        os << "typedef" << endl
           << "no_op_pointer_cache_traits<" << ptr << ">" << endl
           << "pointer_cache_traits;"
           << endl
           << "typedef" << endl
           << "no_op_reference_cache_traits<" << obj << ">" << endl
           << "reference_cache_traits;"
           << endl;
      }
    }

    // callback ()
    //
    os << "static void" << endl
       << "callback (database&, object_type&, callback_event);"
       <<  endl;

    os << "static void" << endl
       << "callback (database&, const object_type&, callback_event);"
       <<  endl;
  }

  os << "};";

  // The rest only applies to dynamic milti-database support.
  //
  if (!multi_dynamic)
    return;

  // pointer_query_columns & query_columns
  //
  if (options.generate_query ())
  {
    // If we don't have object pointers, then also generate
    // query_columns (in this case pointer_query_columns and
    // query_columns are the same and the former inherits from
    // the latter). Otherwise we have to postpone query_columns
    // generation until the second pass to deal with forward-
    // declared objects.
    //
    if (!has_a (c, test_pointer | include_base))
      query_columns_type_->traverse (c);

    pointer_query_columns_type_->traverse (c);
  }

  // object_traits_impl
  //
  os << "template <>" << endl
     << "class " << exp << "access::object_traits_impl< " << type << ", " <<
    "id_common >:" << endl
     << "  public access::object_traits< " << type << " >"
     << "{";

  // We don't need to generate anything else for reuse-abstract objects.
  //
  if (reuse_abst)
  {
    os << "};";
    return;
  }

  os << "public:" << endl;

  if (options.generate_query ())
  {
    // base_traits is needed for query support.
    //
    if (poly_derived)
      os << "typedef object_traits_impl<base_type, id_common> base_traits;"
         << endl;

    // query_base_type
    //
    os << "typedef odb::query_base query_base_type;"
       << endl;
  }

  // function_table_type
  //
  os << "struct function_table_type"
     << "{";

  // persist ()
  //
  os << "void (*persist) (database&, " << (auto_id ? "" : "const ") <<
    "object_type&" << (poly ? ", bool, bool" : "") << ");";

  if (id != 0)
  {
    // find (id)
    //
    if (c.default_ctor ())
      os << "pointer_type (*find1) (database&, const id_type&);";

    // find (id, obj)
    //
    os << "bool (*find2) (database&, const id_type&, object_type&" <<
      (poly ? ", bool" : "") << ");";

    // reload ()
    //
    os << "bool (*reload) (database&, object_type&" <<
      (poly ? ", bool" : "") << ");";

    // update ()
    //
    if (!readonly (c) || poly)
    {
      os << "void (*update) (database&, const object_type&" <<
        (poly ? ", bool, bool" : "") << ");";
    }

    // erase ()
    //
    os << "void (*erase1) (database&, const id_type&" <<
      (poly ? ", bool, bool" : "") << ");";

    os << "void (*erase2) (database&, const object_type&" <<
      (poly ? ", bool, bool" : "") << ");";

    // Sections.
    //
    if (uss.count (user_sections::count_total |
                   user_sections::count_load  |
                   (poly ? user_sections::count_load_empty : 0)) != 0)
      os << "bool (*load_section) (connection&, object_type&, section&" <<
        (poly ? ", const info_type*" : "") << ");";

    if (uss.count (user_sections::count_total  |
                   user_sections::count_update |
                   (poly ? user_sections::count_update_empty : 0)) != 0)
      os << "bool (*update_section) (connection&, const object_type&, " <<
        "const section&" << (poly ? ", const info_type*" : "") << ");";
  }

  if (options.generate_query ())
  {
    if (!options.omit_unprepared ())
      os << "result<object_type> (*query) (database&, const query_base_type&);";

    os << "unsigned long long (*erase_query) (database&, " <<
      "const query_base_type&);";

    if (options.generate_prepared ())
    {
      os << "odb::details::shared_ptr<prepared_query_impl> " <<
        "(*prepare_query) (connection&, const char*, const query_base_type&);";

      os << "odb::details::shared_ptr<result_impl> (*execute_query) ("
        "prepared_query_impl&);";
    }
  }

  os << "};" // function_table_type
     << "static const function_table_type* function_table[database_count];"
     << endl;

  //
  // Forwarding functions.
  //

  // persist ()
  //
  os << "static void" << endl
     << "persist (database&, " << (auto_id ? "" : "const ") << "object_type&);"
     << endl;

  if (id != 0)
  {
    // find (id)
    //
    if (c.default_ctor ())
      os << "static pointer_type" << endl
         << "find (database&, const id_type&);"
         << endl;

    // find (id, obj)
    //
    os << "static bool" << endl
       << "find (database&, const id_type&, object_type&);"
       << endl;

    // reload ()
    //
    os << "static bool" << endl
       << "reload (database&, object_type&);"
       << endl;

    // update ()
    //
    if (!readonly (c) || poly)
    {
      os << "static void" << endl
         << "update (database&, const object_type&);"
         << endl;
    }

    // erase ()
    //
    os << "static void" << endl
       << "erase (database&, const id_type&);"
       << endl;

    os << "static void" << endl
       << "erase (database&, const object_type&);"
       << endl;

    // Sections.
    //
    if (uss.count (user_sections::count_total |
                   user_sections::count_load  |
                   (poly ? user_sections::count_load_empty : 0)) != 0)
      os << "static bool" << endl
         << "load (connection&, object_type&, section&);"
         << endl;

    if (uss.count (user_sections::count_total  |
                   user_sections::count_update |
                   (poly ? user_sections::count_update_empty : 0)) != 0)
      os << "static bool" << endl
         << "update (connection&, const object_type&, const section&);"
         << endl;
  }

  if (options.generate_query ())
  {
    if (!options.omit_unprepared ())
    {
      os << "static result<object_type>" << endl
         << "query (database&, const query_base_type&);"
         << endl;
    }

    os << "static unsigned long long" << endl
       << "erase_query (database&, const query_base_type&);"
       << endl;

    if (options.generate_prepared ())
    {
      os << "static odb::details::shared_ptr<prepared_query_impl>" << endl
         << "prepare_query (connection&, const char*, const query_base_type&);"
         << endl;

      os << "static odb::details::shared_ptr<result_impl>" << endl
         << "execute_query (prepared_query_impl&);"
         << endl;
    }
  }

  os << "};"; // object_traits_impl
}

void header::class1::
traverse_view (type& c)
{
  string const& type (class_fq_name (c));

  os << "// " << class_name (c) << endl
     << "//" << endl;

  // class_traits
  //
  os << "template <>" << endl
     << "struct class_traits< " << type << " >"
     << "{"
     << "static const class_kind kind = class_view;"
     << "};";

  // view_traits
  //
  os << "template <>" << endl
     << "class " << exp << "access::view_traits< " << type << " >"
     << "{"
     << "public:" << endl;

  // view_type & pointer_type
  //
  os << "typedef " << type << " view_type;"
     << "typedef " << c.get<string> ("object-pointer") << " pointer_type;"
     << endl;

  // Generate associated object tags here if we are generating dynamic
  // multi-database support.
  //
  if (multi_dynamic)
  {
    query_tags t;
    t.traverse (c);
  }

  // callback ()
  //
  os << "static void" << endl
     << "callback (database&, view_type&, callback_event);"
     <<  endl;

  os << "};";

  // The rest only applies to dynamic milti-database support.
  //
  if (!multi_dynamic)
    return;

  size_t obj_count (c.get<size_t> ("object-count"));

  // view_traits_impl
  //
  os << "template <>" << endl
     << "class " << exp << "access::view_traits_impl< " << type << ", " <<
    "id_common >:" << endl
     << "  public access::view_traits< " << type << " >"
     << "{"
     << "public:" << endl;

  // query_base_type and query_columns (definition generated by class2).
  //
  os << "typedef odb::query_base query_base_type;"
     << "struct query_columns";

  if (obj_count == 0)
    os << "{"
       << "};";
  else
    os << ";"
       << endl;

  // function_table_type
  //
  os << "struct function_table_type"
     << "{";

  if (!options.omit_unprepared ())
    os << "result<view_type> (*query) (database&, const query_base_type&);"
       << endl;

  if (options.generate_prepared ())
  {
    os << "odb::details::shared_ptr<prepared_query_impl> " <<
      "(*prepare_query) (connection&, const char*, const query_base_type&);"
       << endl;

    os << "odb::details::shared_ptr<result_impl> (*execute_query) ("
      "prepared_query_impl&);"
       << endl;
  }

  os << "};" // function_table_type
     << "static const function_table_type* function_table[database_count];"
     << endl;

  //
  // Forwarding functions.
  //

  if (!options.omit_unprepared ())
    os << "static result<view_type>" << endl
       << "query (database&, const query_base_type&);"
       << endl;

  if (options.generate_prepared ())
  {
    os << "static odb::details::shared_ptr<prepared_query_impl>" << endl
       << "prepare_query (connection&, const char*, const query_base_type&);"
       << endl;

    os << "static odb::details::shared_ptr<result_impl>" << endl
       << "execute_query (prepared_query_impl&);"
       << endl;
  }

  os << "};";
}

namespace header
{
  struct class2: traversal::class_, virtual context
  {
    class2 ()
        : typedefs_ (false),
          query_columns_type_ (false, true, false),
          query_columns_type_inst_ (false, false, true),
          view_query_columns_type_ (true)
    {
      *this >> defines_ >> *this;
      *this >> typedefs_ >> *this;
    }

    virtual void
    traverse (type& c)
    {
      class_kind_type ck (class_kind (c));

      if (ck == class_other ||
          (!options.at_once () && class_file (c) != unit.file ()))
        return;

      names (c);

      switch (ck)
      {
      case class_object: traverse_object (c); break;
      case class_view: traverse_view (c); break;
      default: break;
      }
    }

    void
    traverse_object (type&);

    void
    traverse_view (type&);

  private:
    traversal::defines defines_;
    typedefs typedefs_;

    instance<query_columns_type> query_columns_type_;
    instance<query_columns_type> query_columns_type_inst_;
    instance<view_query_columns_type> view_query_columns_type_;
  };
}

void header::class2::
traverse_object (type& c)
{
  if (options.generate_query ())
  {
    os << "// " << class_name (c) << endl
       << "//" << endl;

    // query_columns
    //
    // If we don't have any pointers, then query_columns is generated
    // in pass 1 (see the comment in class1 for details).
    //
    if (has_a (c, test_pointer | include_base))
      query_columns_type_->traverse (c);

    // Generate extern template declarations.
    //
    query_columns_type_inst_->traverse (c);
  }

  // Move header comment out of if-block if adding any code here.
}

void header::class2::
traverse_view (type& c)
{
  // query_columns
  //
  if (c.get<size_t> ("object-count") != 0)
  {
    os << "// " << class_name (c) << endl
       << "//" << endl;

    view_query_columns_type_->traverse (c);
  }

  // Move header comment out of if-block if adding any code here.
}

namespace header
{
  void
  generate ()
  {
    context ctx;
    ostream& os (ctx.os);

    os << "#include <memory>" << endl
       << "#include <cstddef>" << endl; // std::size_t

    if (ctx.features.polymorphic_object)
      os << "#include <string>" << endl; // For discriminator.

    if (ctx.options.std () >= cxx_version::cxx11)
      os << "#include <utility>" << endl; // move()

    os << endl;

    os << "#include <odb/core.hxx>" << endl
       << "#include <odb/traits.hxx>" << endl
       << "#include <odb/callback.hxx>" << endl
       << "#include <odb/wrapper-traits.hxx>" << endl
       << "#include <odb/pointer-traits.hxx>" << endl;

    // @@ TMP: drop after 2.5.0.
    //
#if 0
    if (ctx.options.std () == cxx_version::cxx98)
    {
      // In case of a boost TR1 implementation, we cannot distinguish
      // between the boost::shared_ptr and std::tr1::shared_ptr usage since
      // the latter is just a using-declaration for the former. To resolve
      // this we will include TR1 traits if the Boost TR1 header is included.
      //
      if (ctx.features.tr1_pointer)
      {
        os << "#include <odb/tr1/wrapper-traits.hxx>" << endl
           << "#include <odb/tr1/pointer-traits.hxx>" << endl;
      }
      else if (ctx.features.boost_pointer)
      {
        os << "#ifdef BOOST_TR1_MEMORY_HPP_INCLUDED" << endl
           << "#  include <odb/tr1/wrapper-traits.hxx>" << endl
           << "#  include <odb/tr1/pointer-traits.hxx>" << endl
           << "#endif" << endl;
      }
    }
#endif

    os << "#include <odb/container-traits.hxx>" << endl;

    if (ctx.features.session_object)
    {
      if (ctx.options.session_type () == "odb::session")
        os << "#include <odb/session.hxx>" << endl;

      os << "#include <odb/cache-traits.hxx>" << endl;
    }
    else
      os << "#include <odb/no-op-cache-traits.hxx>" << endl;

    if (ctx.features.polymorphic_object)
      os << "#include <odb/polymorphic-info.hxx>" << endl;

    if (ctx.options.generate_query ())
    {
      if (ctx.multi_dynamic)
        os << "#include <odb/query-dynamic.hxx>" << endl;

      if (ctx.options.generate_prepared ())
        os << "#include <odb/prepared-query.hxx>" << endl;

      os << "#include <odb/result.hxx>" << endl;

      if (ctx.features.simple_object)
        os << "#include <odb/simple-object-result.hxx>" << endl;

      if (ctx.features.polymorphic_object)
        os << "#include <odb/polymorphic-object-result.hxx>" << endl;

      if (ctx.features.no_id_object)
        os << "#include <odb/no-id-object-result.hxx>" << endl;

      if (ctx.features.view)
        os << "#include <odb/view-image.hxx>" << endl
           << "#include <odb/view-result.hxx>" << endl;
    }

    os << endl
       << "#include <odb/details/unused.hxx>" << endl;

    if (ctx.options.generate_query ())
      os << "#include <odb/details/shared-ptr.hxx>" << endl;

    os << endl;

    os << "namespace odb"
       << "{";

    // Generate common code.
    //
    {
      traversal::unit unit;
      traversal::defines unit_defines;
      typedefs unit_typedefs (false);
      traversal::namespace_ ns;
      class1 c;

      unit >> unit_defines >> ns;
      unit_defines >> c;
      unit >> unit_typedefs >> c;

      traversal::defines ns_defines;
      typedefs ns_typedefs (false);

      ns >> ns_defines >> ns;
      ns_defines >> c;
      ns >> ns_typedefs >> c;

      unit.dispatch (ctx.unit);
    }

    if (ctx.multi_dynamic)
    {
      traversal::unit unit;
      traversal::defines unit_defines;
      typedefs unit_typedefs (false);
      traversal::namespace_ ns;
      class2 c;

      unit >> unit_defines >> ns;
      unit_defines >> c;
      unit >> unit_typedefs >> c;

      traversal::defines ns_defines;
      typedefs ns_typedefs (false);

      ns >> ns_defines >> ns;
      ns_defines >> c;
      ns >> ns_typedefs >> c;

      unit.dispatch (ctx.unit);
    }

    os << "}";
  }
}
