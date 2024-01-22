// file      : odb/relational/header.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/header.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

void relational::header::class1::
traverse_object (type& c)
{
  using semantics::data_member;

  data_member_path* id (id_member (c));
  data_member* idf (id ? id->front () : 0);
  data_member* idb (id ? id->back () : 0);
  bool auto_id (id && auto_ (*id));
  bool base_id (id && &idf->scope () != &c); // Comes from base.

  data_member* opt (context::optimistic (c));

  type* poly_root (polymorphic (c));
  bool poly (poly_root != 0);
  bool poly_derived (poly && poly_root != &c);
  data_member* discriminator (poly ? context::discriminator (*poly_root) : 0);

  bool abst (abstract (c));
  bool reuse_abst (abst && !poly);

  bool versioned (context::versioned (c));

  string const& type (class_fq_name (c));

  // Sections.
  //
  user_sections& uss (c.get<user_sections> ("user-sections"));

  os << "// " << class_name (c) << endl
     << "//" << endl;

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
    "id_" << db << " >:"  << endl
     << "  public access::object_traits< " << type << " >"
     << "{"
     << "public:" << endl;

  object_public_extra_pre (c);

  // For dynamic multi-database support also generate common traits
  // alias (used in query aliasing).
  //
  if (options.generate_query () && multi_dynamic)
  {
    os << "typedef access::object_traits_impl< " << type << ", " <<
      "id_common > common_traits;"
       << endl;
  }

  // Polymorphic root_traits, base_traits, and discriminator_image_type.
  //
  if (poly)
  {
    if (!abst)
      os << "typedef polymorphic_entry<object_type, id_" << db <<
        "> entry_type;";

    os << "typedef object_traits_impl<root_type, id_" << db << "> " <<
      "root_traits;";

    if (poly_derived)
    {
      os << "typedef object_traits_impl<base_type, id_" << db << "> " <<
        "base_traits;"
         << endl;
    }
    else
    {
      os << endl
         << "struct discriminator_image_type"
         << "{";

      discriminator_image_member_->traverse (*discriminator);

      if (opt != 0)
        version_image_member_->traverse (*opt);

      os << "std::size_t version;"
         << "};";
    }
  }

  // id_image_type
  //
  if (id != 0)
  {
    if (base_id)
    {
      if (poly_derived)
        os << "typedef root_traits::id_image_type id_image_type;"
           << endl;
      else
      {
        semantics::class_& b (
          dynamic_cast<semantics::class_&> (idf->scope ()));

        os << "typedef object_traits_impl< " << class_fq_name (b) << ", " <<
          "id_" << db << " >::id_image_type id_image_type;"
           << endl;
      }
    }
    else
    {
      os << "struct id_image_type"
         << "{";

      id_image_member_->traverse (*idb);

      if (opt != 0)
        version_image_member_->traverse (*opt);

      os << "std::size_t version;"
         << "};";
    }
  }

  // Polymorphic map.
  //
  if (poly)
  {
    if (!poly_derived)
      os << "static map_type* map;";

    os << "static const " << (abst ? "abstract_" : "") << "info_type info;"
       << endl;
  }

  // image_type
  //
  image_type_->traverse (c);

  // Extra (container, section) statement cache (forward declaration).
  //
  if (!reuse_abst && id != 0)
    os << "struct extra_statement_cache_type;"
       << endl;

  //
  // Containers (abstract and concrete).
  //

  {
    instance<container_traits> t (c);
    t->traverse (c);
  }

  //
  // Sections (abstract and concrete).
  //

  for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
  {
    instance<section_traits> t (c);
    t->traverse (*i);
  }

  //
  // Query (abstract and concrete).
  //

  if (options.generate_query ())
  {
    // Generate object pointer tags here unless we are generating dynamic
    // multi-database support, in which case they generated in object_traits.
    //
    if (!multi_dynamic && has_a (c, test_pointer | exclude_base))
    {
      query_tags t;   // Not customizable.
      t.traverse (c);
    }
  }

  //
  // Functions (abstract and concrete).
  //

  if (id != 0 || !reuse_abst)
    os << "using object_traits<object_type>::id;"
       << endl;

  if (opt != 0)
    os << "using object_traits<object_type>::version;"
       << endl;

  if (!poly_derived && id != 0)
  {
    if (auto_id)
      os << "static id_type" << endl
         << "id (const id_image_type&);"
         << endl;

    if (options.generate_query ())
      os << "static id_type" << endl
         << "id (const image_type&);"
         << endl;

    if (opt != 0)
      os << "static version_type" << endl
         << "version (const image_type&);"
         << endl;
  }

  // discriminator()
  //
  if (poly && !poly_derived)
    os << "static discriminator_type" << endl
       << "discriminator (const image_type&);"
       << endl;

  // grow ()
  //
  if (generate_grow)
  {
    // For derived classes in a polymorphic hierarchy, grow() will
    // check bases up to the specified depth. If one of the base
    // images has grown, then it will increment its version. But
    // the return value only indicates the state of this image,
    // excluding polymorphic bases (in other words, it is possible
    // that one of the bases has grown but this function returns
    // false).
    //
    os << "static bool" << endl
       << "grow (image_type&," << endl
       << truncated_vector;

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    if (poly_derived)
      os << "," << endl
         << "std::size_t = depth";

    os << ");"
       << endl;
  }

  // bind (image_type)
  //
  os << "static void" << endl
     << "bind (" << bind_vector << "," << endl;

  // If we are a derived type in a polymorphic hierarchy, then
  // we get the the external id binding.
  //
  if (poly_derived)
    os << "const " << bind_vector << " id," << endl
       << "std::size_t id_size," << endl;

  os << "image_type&," << endl
     << db << "::statement_kind";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ");"
     << endl;

  // bind (id_image_type)
  //
  if (id != 0)
  {
    os << "static void" << endl
       << "bind (" << bind_vector << ", id_image_type&" <<
      (opt != 0 ? ", bool bind_version = true" : "") << ");"
       << endl;
  }

  // init (image, object)
  //
  os << "static " << (generate_grow ? "bool" : "void") << endl
     << "init (image_type&," << endl
     << "const object_type&," << endl
     << db << "::statement_kind";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ");"
     << endl;

  // init (object, image)
  //
  os << "static void" << endl
     << "init (object_type&," << endl
     << "const image_type&," << endl
     << "database*";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  if (poly_derived)
    os << "," << endl
       << "std::size_t = depth";

  os << ");"
     << endl;

  // init (id_image, id)
  //
  if (id != 0)
  {
    os << "static void" << endl
       << "init (id_image_type&, const id_type&" <<
      (opt != 0 ? ", const version_type* = 0" : "") << ");"
       << endl;
  }

  if (poly_derived)
  {
    // check_version
    //
    os << "static bool" << endl
       << "check_version (const std::size_t*, const image_type&);"
       << endl;

    // update_version
    //
    os << "static void" << endl
       << "update_version (std::size_t*, const image_type&, " <<
      db << "::binding*);"
       << endl;
  }

  // The rest does not apply to reuse-abstract objects.
  //
  if (reuse_abst)
  {
    object_public_extra_post (c);
    os << "};";
    return;
  }

  column_count_type const& cc (column_count (c));

  // Statements typedefs.
  //
  if (poly)
  {
    if (poly_derived)
      os << "typedef" << endl
         << db << "::polymorphic_derived_object_statements" <<
        "<object_type>" << endl
         << "statements_type;"
         << endl
         << "typedef" << endl
         << db << "::polymorphic_root_object_statements<root_type>" << endl
         << "root_statements_type;"
         << endl;
    else
      os << "typedef" << endl
         << db << "::polymorphic_root_object_statements<object_type>" << endl
         << "statements_type;"
         << endl
         << "typedef statements_type root_statements_type;"
         << endl;
  }
  else
  {
    if (id != 0)
      os << "typedef " << db << "::object_statements<object_type> " <<
        "statements_type;"
         << endl;
    else
      os << "typedef " << db << "::no_id_object_statements<object_type> " <<
        "statements_type;"
         << endl;
  }

  //
  // Query (concrete).
  //

  if (options.generate_query ())
  {
    // query_base_type
    //
    os << "typedef " << db << "::query_base query_base_type;"
       << endl;
  }

  //
  // Containers (concrete).
  //

  //
  // Sections (concrete).
  //

  // column_count
  //
  os << "static const std::size_t column_count = " << cc.total << "UL;"
     << "static const std::size_t id_column_count = " << cc.id << "UL;"
     << "static const std::size_t inverse_column_count = " <<
    cc.inverse << "UL;"
     << "static const std::size_t readonly_column_count = " <<
    cc.readonly << "UL;"
     << "static const std::size_t managed_optimistic_column_count = " <<
    cc.optimistic_managed << "UL;";

  if (poly && !poly_derived)
    os << "static const std::size_t discriminator_column_count = " <<
      cc.discriminator << "UL;";

  os << endl
     << "static const std::size_t separate_load_column_count = " <<
    cc.separate_load << "UL;"
     << "static const std::size_t separate_update_column_count = " <<
    cc.separate_update << "UL;"
     << endl;

  os << "static const bool versioned = " << versioned << ";"
     << endl;

  // Statements.
  //
  os << "static const char persist_statement[];";

  if (id != 0)
  {
    if (poly_derived)
    {
      char const* n (abst ? "1" : "depth");

      os << "static const char* const find_statements[" << n << "];"
         << "static const std::size_t find_column_counts[" << n << "];";
    }
    else
    {
      os << "static const char find_statement[];";

      if (poly)
        os << "static const char find_discriminator_statement[];";
    }

    if (cc.total != cc.id + cc.inverse + cc.readonly + cc.separate_update)
      os << "static const char update_statement[];";

    os << "static const char erase_statement[];";

    if (opt != 0 && !poly_derived)
      os << "static const char optimistic_erase_statement[];";
  }

  if (options.generate_query ())
  {
    os << "static const char query_statement[];"
       << "static const char erase_query_statement[];"
       << endl
       << "static const char table_name[];";
  }

  os << endl;

  //
  // Functions (concrete).
  //

  // persist ()
  //
  os << "static void" << endl
     << "persist (database&, " << (auto_id ? "" : "const ") << "object_type&";

  if (poly)
    os << ", bool top = true, bool dyn = true";

  os << ");"
     << endl;

  if (c.count ("bulk-persist"))
    os << "static void" << endl
       << "persist (database&, " << (auto_id ? "" : "const ") <<
      "object_type**, std::size_t, multiple_exceptions&);"
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
       << "find (database&, const id_type&, object_type&";

    if (poly)
      os << ", bool dyn = true";

    os << ");"
       << endl;

    // reload ()
    //
    os << "static bool" << endl
       << "reload (database&, object_type&";

    if (poly)
      os << ", bool dyn = true";

    os << ");"
       << endl;

    // update ()
    //
    // In case of a polymorphic object, we generate update() even if it is
    // readonly since the potentially-readwrite base will rely on it to
    // initialize the id image.
    //
    //
    if (!readonly (c) || poly)
    {
      os << "static void" << endl
         << "update (database&, const object_type&";

      if (poly)
        os << ", bool top = true, bool dyn = true";

      os << ");"
         << endl;

      if (c.count ("bulk-update"))
        os << "static void" << endl
           << "update (database&, const object_type**, std::size_t, " <<
          "multiple_exceptions&);"
           << endl;
    }

    // erase ()
    //
    os << "static void" << endl
       << "erase (database&, const id_type&";

    if (poly)
      os << ", bool top = true, bool dyn = true";

    os << ");"
       << endl;

    os << "static void" << endl
       << "erase (database&, const object_type&";

    if (poly)
      os << ", bool top = true, bool dyn = true";

    os << ");"
       << endl;

    if (c.count ("bulk-erase"))
    {
      os << "static std::size_t" << endl
         << "erase (database&, const id_type**, std::size_t, " <<
        "multiple_exceptions&);"
         << endl;

      os << "static void" << endl
         << "erase (database&, const object_type**, std::size_t, " <<
        "multiple_exceptions&);"
         << endl;
    }

    // Sections.
    //
    // We treat all polymorphic sections as (potentially) having something
    // to load or to update since we cannot predict what will be added to
    // them in overrides.
    //
    if (uss.count (user_sections::count_total |
                   user_sections::count_load  |
                   (poly ? user_sections::count_load_empty : 0)) != 0)
      os << "static bool" << endl
         << "load (connection&, object_type&, section&" <<
        (poly ? ", const info_type* = 0" : "") << ");"
         << endl;

    if (uss.count (user_sections::count_total  |
                   user_sections::count_update |
                   (poly ? user_sections::count_update_empty : 0)) != 0)
      os << "static bool" << endl
         << "update (connection&, const object_type&, const section&" <<
        (poly ? ", const info_type* = 0" : "") << ");"
         << endl;
  }

  // query ()
  //
  if (options.generate_query ())
  {
    if (!options.omit_unprepared ())
    {
      os << "static result<object_type>" << endl
         << "query (database&, const query_base_type&);"
         << endl;

      if (multi_dynamic)
        os << "static result<object_type>" << endl
           << "query (database&, const odb::query_base&);"
           << endl;
    }

    os << "static unsigned long long" << endl
       << "erase_query (database&, const query_base_type&);"
       << endl;

    if (multi_dynamic)
      os << "static unsigned long long" << endl
         << "erase_query (database&, const odb::query_base&);"
         << endl;

    if (options.generate_prepared ())
    {
      os << "static odb::details::shared_ptr<prepared_query_impl>" << endl
         << "prepare_query (connection&, const char*, const query_base_type&);"
         << endl;

      if (multi_dynamic)
        os << "static odb::details::shared_ptr<prepared_query_impl>" << endl
           << "prepare_query (connection&, const char*, " <<
          "const odb::query_base&);"
           << endl;

      os << "static odb::details::shared_ptr<result_impl>" << endl
         << "execute_query (prepared_query_impl&);"
         << endl;
    }
  }

  object_public_extra_post (c);

  // Implementation details.
  //
  os << "public:" << endl;

  if (id != 0)
  {
    // Load the object image.
    //
    os << "static bool" << endl
       << "find_ (statements_type&," << endl
       << "const id_type*";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    if (poly_derived && !abst)
      os << "," << endl
         << "std::size_t = depth";

    os << ");"
       << endl;

    // Load the rest of the object (containers, etc). Expects the id
    // image in the object statements to be initialized to the object
    // id.
    //
    os << "static void" << endl
       << "load_ (statements_type&," << endl
       << "object_type&," << endl
       << "bool reload";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    if (poly_derived)
      os << "," << endl
         << "std::size_t = depth";

    os << ");"
       << endl;
  }

  // discriminator_ ()
  //
  if (poly && !poly_derived)
  {
    os << "static void" << endl
       << "discriminator_ (statements_type&," << endl
       << "const id_type&," << endl
       << "discriminator_type*";

    if (opt != 0)
      os << "," << endl
         << "version_type* = 0";

    os << ");"
       << endl;
  }

  // Load the dynamic part of the object. Depth inidicates where
  // the dynamic part starts. Expects the id image in the object
  // statements to be initialized to the object id. We don't need
  // it if we are poly-abstract.
  //
  if (poly_derived && !abst)
    os << "static void" << endl
       << "load_ (database&, root_type&, std::size_t);"
       << endl;

  // Image chain manipulation.
  //
  if (poly && need_image_clone && options.generate_query ())
  {
    os << "static root_traits::image_type&" << endl
       << "root_image (image_type&);"
       << endl;

    // Note that the original image is non-const since for some databases
    // the copy "steals" stuff from the original (e.g., LOB descriptors in
    // Oracle).
    //
    os << "static image_type*" << endl
       << "clone_image (image_type&);"
       << endl;

    os << "static void" << endl
       << "copy_image (image_type&, image_type&);"
       << endl;

    os << "static void" << endl
       << "free_image (image_type*);"
       << endl;
  }

  os << "};";


  // object_traits_impl< , id_common>
  //
  // Note that it is not generated for reuse-abstract classes.
  //
  if (options.default_database_specified () &&
      options.default_database () == db)
  {
    os << "template <>" << endl
       << "class access::object_traits_impl< " << type << ", " <<
      "id_common >:" << endl
       << "  public access::object_traits_impl< " << type << ", " <<
      "id_" << db << " >"
       << "{"
       << "};";
  }
}

void relational::header::class1::
traverse_view (type& c)
{
  bool versioned (context::versioned (c));

  string const& type (class_fq_name (c));
  size_t columns (column_count (c).total);
  size_t obj_count (c.get<size_t> ("object-count"));

  os << "// " << class_name (c) << endl
     << "//" << endl;

  // view_traits_impl
  //
  os << "template <>" << endl
     << "class " << exp << "access::view_traits_impl< " << type << ", " <<
    "id_" << db << " >:" << endl
     << "  public access::view_traits< " << type << " >"
     << "{"
     << "public:" << endl;

  view_public_extra_pre (c);

  // For dynamic multi-database support also generate common traits
  // alias (used in query aliasing).
  //
  if (multi_dynamic)
  {
    os << "typedef access::view_traits_impl< " << type << ", " <<
      "id_common > common_traits;"
       << endl;
  }

  // image_type
  //
  image_type_->traverse (c);

  os << "typedef " << db << "::view_statements<view_type> statements_type;"
     << endl;

  //
  // Query.
  //

  // Generate associated object tags here unless we are generating dynamic
  // multi-database support, in which case they generated in object_traits.
  //
  if (!multi_dynamic)
  {
    query_tags t;   // Not customizable.
    t.traverse (c);
  }

  // query_base_type and query_columns (definition generated by class2).
  //
  os << "typedef " << db << "::query_base query_base_type;"
     << "struct query_columns";

  if (obj_count == 0)
    os << "{"
       << "};";
  else
    os << ";"
       << endl;

  os << "static const bool versioned = " << versioned << ";"
     << endl;

  //
  // Functions.
  //

  // grow ()
  //
  if (generate_grow)
  {
    os << "static bool" << endl
       << "grow (image_type&," << endl
       << truncated_vector;

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    os << ")" << (columns != 0 ? ";\n" : "{}");
  }

  // bind (image_type)
  //
  os << "static void" << endl
     << "bind (" << bind_vector << "," << endl
     << "image_type&";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ")" << (columns != 0 ? ";\n" : "{}");

  // init (view, image)
  //
  os << "static void" << endl
     << "init (view_type&," << endl
     << "const image_type&," << endl
     << "database*";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ")" << (columns != 0 ? ";\n" : "{}");

  // column_count
  //
  os << "static const std::size_t column_count = " << columns << "UL;"
     << endl;

  // Statements.
  //
  view_query& vq (c.get<view_query> ("query"));

  if (vq.kind != view_query::runtime)
  {
    os << "static query_base_type" << endl
       << "query_statement (const query_base_type&);"
       << endl;
  }

  //
  // Functions.
  //

  // query ()
  //
  if (!options.omit_unprepared ())
  {
    os << "static result<view_type>" << endl
       << "query (database&, const query_base_type&);"
       << endl;

    if (multi_dynamic)
      os << "static result<view_type>" << endl
         << "query (database&, const odb::query_base&);"
         << endl;
  }

  if (options.generate_prepared ())
  {
    os << "static odb::details::shared_ptr<prepared_query_impl>" << endl
       << "prepare_query (connection&, const char*, const query_base_type&);"
       << endl;

    if (multi_dynamic)
      os << "static odb::details::shared_ptr<prepared_query_impl>" << endl
         << "prepare_query (connection&, const char*, " <<
        "const odb::query_base&);"
         << endl;

    os << "static odb::details::shared_ptr<result_impl>" << endl
       << "execute_query (prepared_query_impl&);"
       << endl;
  }

  view_public_extra_post (c);

  os << "};";

  // view_traits_impl< , id_common>
  //
  if (options.default_database_specified () &&
      options.default_database () == db)
  {
    os << "template <>" << endl
       << "class access::view_traits_impl< " << type << ", " <<
      "id_common >:" << endl
       << "  public access::view_traits_impl< " << type << ", " <<
      "id_" << db << " >"
       << "{"
       << "};";
  }
}

void relational::header::class1::
traverse_composite (type& c)
{
  bool versioned (context::versioned (c));

  string const& type (class_fq_name (c));

  os << "// " << class_name (c) << endl
     << "//" << endl;

  // While composite_value_traits is not used directly by user code, we
  // still need to export it if the generated code for the same database
  // is split into several DLLs.
  //
  os << "template <>" << endl
     << "class " << exp << "access::composite_value_traits< " << type <<
    ", id_" << db << " >"
     << "{"
     << "public:" << endl;

  // value_type
  //
  os << "typedef " << type << " value_type;"
     << endl;

  // image_type
  //
  image_type_->traverse (c);

  // Containers.
  //
  {
    instance<container_traits> t (c);
    t->traverse (c);
  }

  // grow ()
  //
  if (generate_grow)
  {
    os << "static bool" << endl
       << "grow (image_type&," << endl
       << truncated_vector;

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    os << ");"
       << endl;
  }

  // bind (image_type)
  //
  os << "static void" << endl
     << "bind (" << bind_vector << "," << endl
     << "image_type&," << endl
     << db << "::statement_kind";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ");"
     << endl;

  // init (image, value)
  //
  os << "static " << (generate_grow ? "bool" : "void") << endl
     << "init (image_type&," << endl
     << "const value_type&," << endl
     << db << "::statement_kind";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ");"
     << endl;

  // init (value, image)
  //
  os << "static void" << endl
     << "init (value_type&," << endl
     << "const image_type&," << endl
     << "database*";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration&";

  os << ");"
     << endl;

  if (!has_a (c, test_container))
  {
    // get_null (image)
    //
    os << "static bool" << endl
       << "get_null (const image_type&";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    os << ");"
       << endl;

    // set_null (image)
    //
    os << "static void" << endl
       << "set_null (image_type&," << endl
       << db << "::statement_kind";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration&";

    os << ");"
       << endl;
  }

  column_count_type const& cc (column_count (c));
  os << "static const std::size_t column_count = " << cc.total << "UL;";

  os << "};";
}

void relational::header::
generate ()
{
  context ctx;
  ostream& os (ctx.os);

  instance<include> i;
  i->generate ();

  os << "namespace odb"
     << "{";

  {
    traversal::unit unit;
    traversal::defines unit_defines;
    typedefs unit_typedefs (false);
    traversal::namespace_ ns;
    instance<class1> c;

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

  {
    traversal::unit unit;
    traversal::defines unit_defines;
    typedefs unit_typedefs (false);
    traversal::namespace_ ns;
    instance<class2> c;

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
