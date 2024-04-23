// file      : odb/relational/source.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <map>

#include <odb/gcc.hxx>

#include <odb/lookup.hxx>
#include <odb/cxx-lexer.hxx>

#include <odb/relational/source.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

void relational::source::class_::
traverse_object (type& c)
{
  using semantics::data_member;

  data_member_path* id (id_member (c));
  data_member* idf (id ? id->front () : 0);
  data_member* idb (id ? id->back () : 0);
  bool auto_id (id && auto_ (*id));
  bool base_id (id && &idf->scope () != &c); // Comes from base.

  data_member* opt (optimistic (c));

  type* poly_root (polymorphic (c));
  bool poly (poly_root != 0);
  bool poly_derived (poly && poly_root != &c);
  type* poly_base (poly_derived ? &polymorphic_base (c) : 0);
  size_t poly_depth (poly_derived ? polymorphic_depth (c) : 1);
  data_member* discriminator (poly ? context::discriminator (*poly_root) : 0);

  bool abst (abstract (c));
  bool reuse_abst (abst && !poly);
  bool readonly (context::readonly (c));

  bool grow (false);
  bool grow_id (false);

  if (generate_grow)
  {
    grow = context::grow (c);
    grow_id = (id ? context::grow (*idb) : false) ||
      (opt ? context::grow (*opt) : false);
  }

  column_count_type const& cc (column_count (c));
  bool versioned (context::versioned (c));

  // Schema name as a string literal or empty.
  //
  string schema_name (options.schema_name ()[db]);
  if (!schema_name.empty ())
    schema_name = strlit (schema_name);

  string const& type (class_fq_name (c));
  string traits ("access::object_traits_impl< " + type + ", id_" +
                 db.string () + " >");

  user_sections& uss (c.get<user_sections> ("user-sections"));
  user_sections* buss (poly_base != 0
                       ? &poly_base->get<user_sections> ("user-sections")
                       : 0);

  os << "// " << class_name (c) << endl
     << "//" << endl
     << endl;

  object_extra (c);

  //
  // Query (abstract and concrete).
  //

  // query_columns
  //
  if (options.generate_query ())
    query_columns_type_->traverse (c);

  // Statement cache (definition).
  //
  if (!reuse_abst && id != 0)
  {
    bool sections (false);
    bool containers (has_a (c, test_container));

    os << "struct " << traits << "::extra_statement_cache_type"
       << "{";

    instance<container_cache_members> cm;
    cm->traverse (c);

    if (containers)
      os << endl;

    instance<section_cache_members> sm;
    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      // Skip the special version update section in reuse inheritance (we
      // always treat it as abstract).
      //
      if (i->special == user_section::special_version && !poly)
        continue;

      // Generate an entry for a readonly section in optimistic
      // polymorphic root since it can be overridden and we may
      // need to update the version.
      //
      if (!i->empty () || (poly && i->optimistic ()))
      {
        sm->traverse (*i);
        sections = true;
      }
    }

    if (sections)
      os << endl;

    os << "extra_statement_cache_type (" << endl
       << db << "::connection&" << (containers || sections ? " c" : "") <<
      "," << endl
       << "image_type&" << (sections ? " im" : "") << "," << endl
       << "id_image_type&" << (sections ? " idim" : "") << "," << endl
       << db << "::binding&" << (containers || sections ? " id" : "") <<
      "," << endl
       << db << "::binding&" << (sections ? " idv" : "");

    extra_statement_cache_extra_args (containers, sections);

    os << ")";

    instance<container_cache_init_members> cim;
    cim->traverse (c);

    instance<section_cache_init_members> sim (!containers);
    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      if (i->special == user_section::special_version && !poly)
        continue;

      if (!i->empty () || (poly && i->optimistic ()))
        sim->traverse (*i);
    }

    os << "{"
       << "}"
       << "};";
  }

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
  // Functions (abstract and concrete).
  //

  // id(), version()
  //
  if (!poly_derived && id != 0 && !base_id)
  {
    // id (id_image_type)
    //
    if (auto_id)
    {
      os << traits << "::id_type" << endl
         << traits << "::" << endl
         << "id (const id_image_type& i)"
         << "{"
         << db << "::database* db (0);"
         << "ODB_POTENTIALLY_UNUSED (db);"
         << endl
         << "id_type id;";
      init_id_value_member_id_image_->traverse (*idb);
      os << "return id;"
         << "}";
    }

    // id (image)
    //
    if (options.generate_query ())
    {
      os << traits << "::id_type" << endl
         << traits << "::" << endl
         << "id (const image_type& i)"
         << "{"
         << db << "::database* db (0);"
         << "ODB_POTENTIALLY_UNUSED (db);"
         << endl
         << "id_type id;";

      // Handle nested id.
      //
      if (id->size () > 1)
      {
        string var;

        for (data_member_path::const_iterator i (id->begin ());
             i != id->end ();
             ++i)
        {
          // The same logic as in member_base.
          //
          if (!var.empty ())
            var += "value."; // Composite.

          string const& name ((*i)->name ());
          var += name;

          if (name[name.size () - 1] != '_')
            var += '_';
        }

        instance<init_value_member> t ("id", var);
        t->traverse (*idb);
      }
      else
        init_id_value_member_->traverse (*idb);

      os << "return id;"
         << "}";
    }

    // version (image)
    //
    if (opt != 0)
    {
      os << traits << "::version_type" << endl
         << traits << "::" << endl
         << "version (const image_type& i)"
         << "{"
         << "version_type v;";
      init_version_value_member_->traverse (*opt);
      os << "return v;"
         << "}";
    }
  }

  // discriminator(image)
  //
  if (poly && !poly_derived)
  {
    os << traits << "::discriminator_type" << endl
       << traits << "::" << endl
       << "discriminator (const image_type& i)"
       << "{"
       << db << "::database* db (0);"
       << "ODB_POTENTIALLY_UNUSED (db);"
       << endl
       << "discriminator_type d;";
    init_discriminator_value_member_->traverse (*discriminator);
    os << "return d;"
       << "}";
  }

  // grow ()
  //
  if (generate_grow)
  {
    os << "bool " << traits << "::" << endl
       << "grow (image_type& i," << endl
       << truncated_vector << " t";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration& svm";

    if (poly_derived)
      os << "," << endl
         << "std::size_t d";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (i);"
       << "ODB_POTENTIALLY_UNUSED (t);";

    if (versioned)
      os << "ODB_POTENTIALLY_UNUSED (svm);";

    os << endl
       << "bool grew (false);"
       << endl;

    index_ = 0;

    if (poly_derived)
    {
      // Select column count for this class.
      //
      size_t cols (cc.total - cc.id);

      os << "// " << class_name (*poly_base) << " base" << endl
         << "//" << endl
         << "if (--d != 0)"
         << "{"
         << "if (base_traits::grow (*i.base, " <<
        "t + " << cols << "UL" <<
        (context::versioned (*poly_base) ? ", svm" : "") <<
        (poly_base != poly_root ? ", d" : "") << "))" << endl
         << "i.base->version++;"
         << "}";
    }
    else
      inherits (c, grow_base_inherits_);

    names (c, grow_member_names_);

    os << "return grew;"
       << "}";
  }

  // bind (image_type)
  //
  os << "void " << traits << "::" << endl
     << "bind (" << bind_vector << " b," << endl;

  // If we are a derived type in a polymorphic hierarchy, then
  // we get the external id binding.
  //
  if (poly_derived)
    os << "const " << bind_vector << " id," << endl
       << "std::size_t id_size," << endl;

  os << "image_type& i," << endl
     << db << "::statement_kind sk";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration& svm";

  os << ")"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (sk);";

  if (versioned)
    os << "ODB_POTENTIALLY_UNUSED (svm);";

  os << endl
     << "using namespace " << db << ";"
     << endl;

  if (readonly)
    os << "assert (sk != statement_update);"
       << endl;

  os << "std::size_t n (0);"
     << endl;

  if (poly_derived)
  {
    // The id reference comes first in the insert statement.
    //
    os << "// " << idf->name () << endl
       << "//" << endl
       << "if (sk == statement_insert)"
       << "{"
       << "if (id != 0)" << endl
       << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
       << "n += id_size;" // Not in if for "id unchanged" optimization.
       << "}";
  }
  else
    inherits (c, bind_base_inherits_);

  names (c, bind_member_names_);

  if (poly_derived)
  {
    // The id reference comes last in the update statement.
    //
    if (!readonly)
      os << "// " << idf->name () << endl
         << "//" << endl
         << "if (sk == statement_update)"
         << "{"
         << "if (id != 0)" << endl
         << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
         << "n += id_size;" // Not in if for "id unchanged" optimization.
         << "}";

    // Bind the image chain for the select statement. Seeing that
    // this is the last statement in the function, we don't care
    // about updating n.
    //
    os << "// " << class_name (*poly_base) << " base" << endl
       << "//" << endl
       << "if (sk == statement_select)" << endl
       << "base_traits::bind (b + n, ";

    if (poly_base != poly_root)
      os << "id, id_size, ";

    os << "*i.base, sk" <<
      (context::versioned (*poly_base) ? ", svm" : "") << ");";
  }

  os << "}";

  // bind (id_image_type)
  //
  if (!poly_derived && id != 0 && !base_id)
  {
    os << "void " << traits << "::" << endl
       << "bind (" << bind_vector << " b, id_image_type& i" <<
      (opt != 0 ? ", bool bv" : "") << ")"
       << "{"
       << "std::size_t n (0);";

    if (composite_wrapper (utype (*id)))
      os << db << "::statement_kind sk (" << db << "::statement_select);";

    bind_id_member_->traverse (*idb);

    if (opt != 0)
    {
      os << "if (bv)"
         << "{"
         << "n += " << column_count (c).id << ";"
         << endl;

      bind_version_member_->traverse (*opt);
      os << "}";
    }

    os << "}";
  }

  // init (image, object)
  //
  os << (generate_grow ? "bool " : "void ") << traits << "::" << endl
     << "init (image_type& i," << endl
     << "const object_type& o," << endl
     << db << "::statement_kind sk";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration& svm";

  os << ")"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (i);"
     << "ODB_POTENTIALLY_UNUSED (o);"
     << "ODB_POTENTIALLY_UNUSED (sk);";

  if (versioned)
    os << "ODB_POTENTIALLY_UNUSED (svm);";

  os << endl
     << "using namespace " << db << ";"
     << endl;

  if (readonly)
    os << "assert (sk != statement_update);"
       << endl;

  init_image_pre (c);

  if (generate_grow)
    os << "bool grew (false);"
       << endl;

  if (!poly_derived)
    inherits (c, init_image_base_inherits_);

  names (c, init_image_member_names_);

  if (generate_grow)
    os << "return grew;";

  os << "}";

  // init (object, image)
  //
  os << "void " << traits << "::" << endl
     << "init (object_type& o," << endl
     << "const image_type& i," << endl
     << "database* db";

  if (versioned)
    os << "," << endl
       << "const schema_version_migration& svm";

  if (poly_derived)
    os << "," << endl
       << "std::size_t d";

  os << ")"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (o);"
     << "ODB_POTENTIALLY_UNUSED (i);"
     << "ODB_POTENTIALLY_UNUSED (db);";

  if (versioned)
    os << "ODB_POTENTIALLY_UNUSED (svm);";

  os << endl;

  if (poly_derived)
  {
    os << "// " << class_name (*poly_base) << " base" << endl
       << "//" << endl
       << "if (--d != 0)" << endl
       << "base_traits::init (o, *i.base, db" <<
      (context::versioned (*poly_base) ? ", svm" : "") <<
      (poly_base != poly_root ? ", d" : "") << ");"
       << endl;
  }
  else
    inherits (c, init_value_base_inherits_);

  names (c, init_value_member_names_);

  os << "}";

  // init (id_image, id)
  //
  if (id != 0 && !base_id)
  {
    os << "void " << traits << "::" << endl
       << "init (id_image_type& i, const id_type& id" <<
      (opt != 0 ? ", const version_type* v" : "") << ")"
       << "{";

    if (grow_id)
      os << "bool grew (false);";

    if (composite_wrapper (utype (*id)))
      os << db << "::statement_kind sk (" << db << "::statement_select);";

    init_id_image_member_->traverse (*idb);

    if (opt != 0)
    {
      // Here we rely on the fact that init_image_member
      // always wraps the statements in a block.
      //
      os << "if (v != 0)";
      init_version_image_member_->traverse (*opt);
    }

    if (grow_id)
      os << "if (grew)" << endl
         << "i.version++;";

    os << "}";
  }

  // The rest does not apply to reuse-abstract objects.
  //
  if (reuse_abst)
    return;

  //
  // Containers (concrete).
  //

  //
  // Sections (concrete).
  //

  // Polymorphic map.
  //
  if (poly)
  {
    if (!poly_derived)
      os << traits << "::map_type*" << endl
         << traits << "::map;"
         << endl;

    // Sections. Do we have any new sections or overrides?
    //
    bool sections (
      uss.count (user_sections::count_new             |
                 user_sections::count_override        |
                 user_sections::count_load            |
                 user_sections::count_update          |
                 user_sections::count_special_version |
                 (poly ? user_sections::count_optimistic : 0)) != 0);
    if (sections)
    {
      string const& fn (flat_name (type));

      size_t n (uss.count (user_sections::count_total |
                           user_sections::count_all |
                           user_sections::count_special_version));

      map<size_t, user_section*> m;
      for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
        m[i->index] = &*i;

      os << "static const "<< traits << "::" << (abst ? "abstract_" : "") <<
        "info_type::section_functions" << endl
         << "section_functions_for_" << fn << "[] ="
         << "{";

      for (size_t i (0); i != n; ++i)
      {
        os << (i != 0 ? "," : "") << "{";

        map<size_t, user_section*>::iterator j (m.find (i));

        if (j != m.end ())
        {
          user_section& s (*j->second);
          string n (public_name (*s.member));

          // load
          //
          if (s.load_empty ())
            os << "0";
          else
            os << "&odb::section_load_impl< " << type << ", id_" << db <<
              ", " << traits << "::" << n << "_traits >";

          os << "," << endl;

          // update
          //
          // Generate an entry for a readonly section in optimistic
          // polymorphic root since it can be overridden and we may
          // need to update the version.
          //
          if (s.update_empty () && !(poly && s.optimistic ()))
            os << "0";
          else
            os << "&odb::section_update_impl< " << type << ", id_" << db <<
              ", " << traits << "::" << n << "_traits >";
        }
        else
          os << "0," << endl
             << "0";

        os << "}";
      }

      os << "};";

      os << "static const "<< traits << "::" << (abst ? "abstract_" : "") <<
        "info_type::section_list" << endl
         << "section_list_for_" << fn << " ="
         << "{"
         << n << "UL," << endl
         << "section_functions_for_" << fn
         << "};";
    }

    //
    //
    os << "const " << traits << "::" << (abst ? "abstract_" : "") <<
      "info_type" << endl
       << traits << "::info (" << endl
       << "typeid (" << type << ")," << endl;

    if (poly_derived)
      os << "&object_traits_impl< " << class_fq_name (*poly_base) <<
        ", id_" << db << " >::info," << endl;
    else
      os << "0," << endl;

    // Sections.
    //
    if (sections)
      os << "&section_list_for_" << flat_name (type);
    else
      os << "0";

    if (!abst)
    {
      os << "," << endl
         << strlit (string (type, 2, string::npos)) << "," << endl
         << "&odb::create_impl< " << type << " >," << endl
         << "&odb::dispatch_impl< " << type << ", id_" << db << " >," << endl;

      if (poly_derived)
        os << "&statements_type::delayed_loader";
      else
        os << "0";
    }

    os << ");"
       << endl;

    if (!abst)
      os << "static const " << traits << "::entry_type" << endl
         << "polymorphic_entry_for_" << flat_name (type) << ";"
         << endl;
  }

  //
  // Statements.
  //
  bool update_columns (
    cc.total != cc.id + cc.inverse + cc.readonly + cc.separate_update);

  qname table (table_name (c));
  string qtable (quote_id (table));

  // persist_statement
  //
  {
    string sep (versioned ? "\n" : " ");

    statement_columns sc;
    {
      statement_kind sk (statement_insert); // Imperfect forwarding.
      instance<object_columns> ct (sk, sc);
      ct->traverse (c);
      process_statement_columns (sc, statement_insert, versioned);
    }

    bool dv (sc.empty ()); // The DEFAULT VALUES syntax.

    os << "const char " << traits << "::persist_statement[] =" << endl
       << strlit ("INSERT INTO " + qtable + sep) << endl;

    for (statement_columns::const_iterator b (sc.begin ()), i (b),
           e (sc.end ()); i != e;)
    {
      string s;

      if (i == b)
        s += '(';
      s += i->column;
      s += (++i != e ? ',' : ')');
      s += sep;

      os << strlit (s) << endl;
    }

    instance<query_parameters> qp (statement_insert, table);

    string extra (persist_statement_extra (c, *qp, persist_after_columns));

    if (!extra.empty ())
      os << strlit (extra + sep) << endl;

    string values;
    if (!dv)
    {
      os << strlit ("VALUES" + sep) << endl;

      values += '(';
      instance<persist_statement_params> pt (values, *qp, sep);
      pt->traverse (c);
      values += ')';
    }
    else
      values = "DEFAULT VALUES";

    extra = persist_statement_extra (c, *qp, persist_after_values);

    if (!extra.empty ())
      values += sep;

    os << strlit (values);

    if (!extra.empty ())
      os << endl
         << strlit (extra);

    os << ";"
       << endl;
  }

  // Index of the first empty SELECT statement in poly-derived
  // statement list. All subsequent statements are also empty.
  // The first statement can never be empty (contains id and
  // type_id).
  //
  size_t empty_depth (0);

  if (id != 0)
  {
    string sep (versioned ? "\n" : " ");

    instance<object_columns_list> id_cols;
    id_cols->traverse (*id);

    std::vector<size_t> find_column_counts (abst ? 1 : poly_depth);

    // find_statement
    //
    if (poly_derived)
      os << "const char* const " << traits << "::find_statements[] ="
         << "{";
    else
      os << "const char " << traits << "::find_statement[] =" << endl;

    for (size_t d (poly_depth); d != 0;)
    {
      statement_columns sc;
      {
        statement_kind sk (statement_select); // Imperfect forwarding.
        object_section* s (&main_section); // Imperfect forwarding.
        instance<object_columns> t (qtable, sk, sc, d, s);
        t->traverse (c);
        process_statement_columns (sc, statement_select, versioned);
        find_column_counts[poly_depth - d] = sc.size ();
      }

      if (sc.size () != 0)
      {
        os << strlit ("SELECT" + sep) << endl;

        for (statement_columns::const_iterator i (sc.begin ()),
               e (sc.end ()); i != e;)
        {
          string const& c (i->column);
          os << strlit (c + (++i != e ? "," : "") + sep) << endl;
        }

        os << strlit ("FROM " + qtable + sep) << endl;

        if (d != 1)
        {
          bool f (false);    // @@ (im)perfect forwarding
          size_t d1 (d - 1); //@@ (im)perfect forward.
          instance<polymorphic_object_joins> j (c, f, d1);
          j->traverse (polymorphic_base (c));

          for (strings::const_iterator i (j->begin ()); i != j->end (); ++i)
            os << strlit (*i + sep) << endl;
        }

        {
          // For the find statement, don't join section members.
          //
          bool f (false); // @@ (im)perfect forwarding
          object_section* s (&main_section); // @@ (im)perfect forwarding
          instance<object_joins> j (c, f, d, s);
          j->traverse (c);

          for (strings::const_iterator i (j->begin ()); i != j->end (); ++i)
            os << strlit (*i + sep) << endl;
        }

        string where ("WHERE ");
        instance<query_parameters> qp (statement_select, table);
        for (object_columns_list::iterator b (id_cols->begin ()), i (b);
             i != id_cols->end (); ++i)
        {
          if (i != b)
            where += " AND ";

          where += qtable + "." + quote_id (i->name) + "=" +
            convert_to (qp->next (*i), i->type, *i->member);
        }

        os << strlit (where);
      }
      else
        os << strlit (""); // Empty SELECT statement.

      if (abst)
        break;

      if (--d != 0)
        os << "," << endl
           << endl;
    }

    if (poly_derived)
      os << "};";
    else
      os << ";"
         << endl;

    // find_column_counts
    //
    if (poly_derived)
    {
      os << "const std::size_t " << traits << "::find_column_counts[] ="
         << "{";

      for (std::vector<size_t>::iterator b (find_column_counts.begin ()),
             i (b), e (find_column_counts.end ()); i != e;)
      {
        os << *i << "UL";

        if (*i == 0 && empty_depth == 0)
          empty_depth = i - b;

        if (++i != e)
          os << ',' << endl;
      }

      os << "};";
    }

    // find_discriminator_statement
    //
    if (poly && !poly_derived)
    {
      statement_columns sc;
      {
        statement_kind sk (statement_select); // Imperfect forwarding.
        instance<object_columns> t (qtable, sk, sc);
        t->traverse (*discriminator);

        if (opt != 0)
          t->traverse (*opt);

        process_statement_columns (sc, statement_select, false);
      }

      os << "const char " << traits << "::" << endl
         << "find_discriminator_statement[] =" << endl
         << strlit ("SELECT ") << endl;

      for (statement_columns::const_iterator i (sc.begin ()),
             e (sc.end ()); i != e;)
      {
        string const& c (i->column);
        os << strlit (c + (++i != e ? ", " : " ")) << endl;
      }

      os << strlit ("FROM " + qtable + " ") << endl;

      string where ("WHERE ");
      instance<query_parameters> qp (statement_select, table);
      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        if (i != b)
          where += " AND ";

        where += qtable + "." + quote_id (i->name) + "=" +
          convert_to (qp->next (*i), i->type, *i->member);
      }

      os << strlit (where) << ";"
         << endl;
    }

    // update_statement
    //
    if (update_columns)
    {
      string sep (versioned ? "\n" : " ");

      instance<query_parameters> qp (statement_update, table);

      statement_columns sc;
      {
        query_parameters* p (qp.get ()); // Imperfect forwarding.
        statement_kind sk (statement_update); // Imperfect forwarding.
        object_section* s (&main_section); // Imperfect forwarding.
        instance<object_columns> t (sk, sc, p, s);
        t->traverse (c);
        process_statement_columns (sc, statement_update, versioned);
      }

      os << "const char " << traits << "::update_statement[] =" << endl
         << strlit ("UPDATE " + qtable + sep) << endl
         << strlit ("SET" + sep) << endl;

      for (statement_columns::const_iterator i (sc.begin ()),
             e (sc.end ()); i != e;)
      {
        string const& c (i->column);
        os << strlit (c + (++i != e ? "," : "") + sep) << endl;
      }

      // This didn't work out: cannot change the identity column.
      //
      //if (sc.empty ())
      //{
      //  // We can end up with nothing to set if we need to "touch" a row
      //  // in order to increment its optimistic concurrency version. In
      //  // this case just do a dummy assignment based on the id column.
      //  //
      //  string const& c (quote_id (id_cols->begin ()->name));
      //  os << strlit (c + "=" + c) << endl;
      //}

      string extra (update_statement_extra (c));

      if (!extra.empty ())
        os << strlit (extra + sep) << endl;

      string where ("WHERE ");
      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        if (i != b)
          where += " AND ";

        where += quote_id (i->name) + "=" +
          convert_to (qp->next (*i), i->type, *i->member);
      }

      // Top-level version column.
      //
      if (opt != 0 && !poly_derived)
      {
        string name (column_qname (*opt, column_prefix ()));
        string type (column_type (*opt));

        where += " AND " + name + "=" +
          convert_to (qp->next (*opt, name, type), type, *opt);
      }

      os << strlit (where) << ";"
         << endl;
    }

    // erase_statement
    //
    {
      instance<query_parameters> qp (statement_delete, table);
      os << "const char " << traits << "::erase_statement[] =" << endl
         << strlit ("DELETE FROM " + qtable + " ") << endl;

      string where ("WHERE ");
      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        if (i != b)
          where += " AND ";

        where += quote_id (i->name) + "=" +
          convert_to (qp->next (*i), i->type, *i->member);
      }

      os << strlit (where) << ";"
         << endl;
    }

    if (opt != 0 && !poly_derived)
    {
      instance<query_parameters> qp (statement_delete, table);

      os << "const char " << traits << "::optimistic_erase_statement[] " <<
        "=" << endl
         << strlit ("DELETE FROM " + qtable + " ") << endl;

      string where ("WHERE ");
      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        if (i != b)
          where += " AND ";

        where += quote_id (i->name) + "=" +
          convert_to (qp->next (*i), i->type, *i->member);
      }

      // Top-level version column.
      //
      string name (column_qname (*opt, column_prefix ()));
      string type (column_type (*opt));

      where += " AND " + name + "=" +
        convert_to (qp->next (*opt, name, type), type, *opt);

      os << strlit (where) << ";"
         << endl;
    }
  }

  // query_statement
  //
  bool query_optimize (false);
  if (options.generate_query ())
  {
    // query_statement
    //
    strings joins;
    if (poly_depth != 1)
    {
      bool t (true);             //@@ (im)perfect forwarding
      size_t d (poly_depth - 1); //@@ (im)perfect forward.
      instance<polymorphic_object_joins> j (c, t, d);
      j->traverse (polymorphic_base (c));
      joins = j->joins;
    }

    if (id != 0)
    {
      // For the query statement we also join section members so that they
      // can be used in the WHERE clause.
      //
      bool t (true); //@@ (im)perfect forwarding
      instance<object_joins> j (c, t, poly_depth);
      j->traverse (c);
      joins.insert (joins.end (), j->begin (), j->end ());
    }

    query_optimize = !joins.empty ();

    statement_columns sc;
    {
      statement_kind sk (statement_select); //@@ Imperfect forwarding.
      object_section* s (&main_section); //@@ Imperfect forwarding.
      instance<object_columns> oc (qtable, sk, sc, poly_depth, s);
      oc->traverse (c);
      process_statement_columns (
        sc, statement_select, versioned || query_optimize);
    }

    string sep (versioned || query_optimize ? "\n" : " ");

    os << "const char " << traits << "::query_statement[] =" << endl
       << strlit ("SELECT" + sep) << endl;

    for (statement_columns::const_iterator i (sc.begin ()),
           e (sc.end ()); i != e;)
    {
      string const& c (i->column);
      os << strlit (c + (++i != e ? "," : "") + sep) << endl;
    }

    string prev ("FROM " + qtable);

    for (strings::const_iterator i (joins.begin ()); i != joins.end (); ++i)
    {
      os << strlit (prev + sep) << endl;
      prev = *i;
    }

    os << strlit (prev) << ";"
       << endl;

    // erase_query_statement
    //
    os << "const char " << traits << "::erase_query_statement[] =" << endl
       << strlit ("DELETE FROM " + qtable) << ";"
       << endl;

    // table_name
    //
    os << "const char " << traits << "::table_name[] =" << endl
       << strlit (qtable) << ";" // Use quoted name.
       << endl;
  }

  // persist ()
  //
  size_t persist_containers (has_a (c, test_straight_container));
  bool persist_versioned_containers (
    persist_containers >
    has_a (c,
           test_straight_container |
           exclude_deleted | exclude_added | exclude_versioned));

  os << "void " << traits << "::" << endl
     << "persist (database& db, " << (auto_id ? "" : "const ") <<
    "object_type& obj";

  if (poly)
    os << ", bool top, bool dyn";

  os << ")"
     << "{";

  if (poly)
    os << "ODB_POTENTIALLY_UNUSED (top);"
       << endl;

  os << "using namespace " << db << ";"
     << endl;

  if (poly)
    os << "if (dyn)" << endl
       << "{"
       << "const std::type_info& t (typeid (obj));"
       << endl
       << "if (t != info.type)"
       << "{"
       << "const info_type& pi (root_traits::map->find (t));"
       << "pi.dispatch (info_type::call_persist, db, &obj, 0);"
       << "return;"
       << "}"
       << "}";

  // If we are database-poly-abstract but not C++-abstract, then make
  // sure we are not trying to persist an instance of an abstract class.
  //
  if (abst && !c.abstract ())
    os << "if (top)" << endl
       << "throw abstract_class ();"
       << endl;

  os << db << "::connection& conn (" << endl
     << db << "::transaction::current ().connection (db));"
     << "statements_type& sts (" << endl
     << "conn.statement_cache ().find_object<object_type> ());";

  if (versioned ||
      persist_versioned_containers ||
      uss.count (user_sections::count_new |
                 user_sections::count_all |
                 user_sections::count_versioned_only) != 0)
    os << "const schema_version_migration& svm (" <<
      "sts.version_migration (" << schema_name << "));";

  os << endl;

  // Call callback (pre_persist).
  //
  if (!abst) // If we are poly-abstract, then top will always be false.
  {
    if (poly)
      os << "if (top)" << endl;

    os << "callback (db," << endl
       << (auto_id ? "static_cast<const object_type&> (obj)," : "obj,") << endl
       << "callback_event::pre_persist);"
       << endl;
  }

  // Call our base if we are a derived type in a polymorphic
  // hierarchy.
  //
  if (poly_derived)
    os << "base_traits::persist (db, obj, false, false);"
       << endl;

  os << "image_type& im (sts.image ());"
     << "binding& imb (sts.insert_image_binding ());";

  if (poly_derived)
    os << "const binding& idb (sts.id_image_binding ());";

  os << endl;

  if (generate_grow)
    os << "if (";

  os << "init (im, obj, statement_insert" << (versioned ? ", svm" : "") << ")";

  if (generate_grow)
    os << ")" << endl
       << "im.version++";

  os << ";"
     << endl;

  if (!poly_derived && auto_id && insert_send_auto_id)
  {
    string const& n (idf->name ());
    string var ("im." + n + (n[n.size () - 1] == '_' ? "" : "_"));
    init_auto_id (*idf, var); // idf == idb, since auto
  }

  os << "if (";

  if (poly_derived)
    os << "idb.version != sts.insert_id_binding_version () ||" << endl;

  os << "im.version != sts.insert_image_version () ||" << endl
     << "imb.version == 0)"
     << "{"
     << "bind (imb.bind, ";

  if (poly_derived)
    os << "idb.bind, idb.count, ";

  os << "im, statement_insert" << (versioned ? ", svm" : "") << ");";

  if (poly_derived)
    os << "sts.insert_id_binding_version (idb.version);";

  os << "sts.insert_image_version (im.version);"
     << "imb.version++;"
     << "}";

  // Bind id image since that's where the returned id and/or version will
  // be extracted.
  //
  if (!poly_derived)
  {
    bool bv (opt != 0 && optimistic_insert_bind_version (*opt));
    if (bv || auto_id)
    {
      os << "{"
         << "id_image_type& i (sts.id_image ());"
         << "binding& b (sts.id_image_binding ());"
         << "if (i.version != sts.id_image_version () || b.version == 0)"
         << "{"
         << "bind (b.bind, i);"
         << "sts.id_image_version (i.version);"
         << "b.version++;";
      if (opt != 0)
        os << "sts.optimistic_id_image_binding ().version++;";
      os << "}"
         << "}";
    }
  }

  os << "insert_statement& st (sts.persist_statement ());"
     << "if (!st.execute ())" << endl
     << "throw object_already_persistent ();"
     << endl;

  if (!poly_derived && auto_id) // idf == idb since auto
  {
    set_member (*idf, "obj", "id (sts.id_image ())", "db", "id_type");
    os << endl;
  }

  // Set the optimistic concurrency version in the object member.
  //
  if (opt != 0 && !poly_derived)
  {
    // If we don't have auto id, then obj is a const reference.
    //
    set_member (*opt,
                (auto_id ? "obj" : "const_cast<object_type&> (obj)"),
                optimistic_version_init (*opt),
                "", // No database.
                "version_type");
    os << endl;
  }

  // Initialize id_image and binding if we are a root of a polymorphic
  // hierarchy or if we have non-inverse containers.
  //
  if (!poly_derived && (poly || persist_containers))
  {
    // If this is a polymorphic root without containers, then we only
    // need to do this if we are not a top-level call. If we are poly-
    // abstract, then top will always be false.
    //
    if (poly && !persist_containers && !abst)
      os << "if (!top)"
         << "{";

    os << "id_image_type& i (sts.id_image ());"
       << "init (i, id (obj));"
       << endl
       << "binding& idb (sts.id_image_binding ());"
       << "if (i.version != sts.id_image_version () || idb.version == 0)"
       << "{"
       << "bind (idb.bind, i);"
       << "sts.id_image_version (i.version);"
       << "idb.version++;";
    if (opt != 0)
      os << "sts.optimistic_id_image_binding ().version++;";
    os << "}";

    if (poly && !persist_containers && !abst)
      os << "}";
  }

  if (persist_containers)
  {
    os << "extra_statement_cache_type& esc (sts.extra_statement_cache ());"
       << endl;

    instance<container_calls> t (container_calls::persist_call);
    t->traverse (c);
  }

  // Reset sections: loaded, unchanged.
  //
  for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
  {
    // Skip special sections.
    //
    if (i->special == user_section::special_version)
      continue;

    // Skip overridden sections; they have been reset by the base.
    //
    if (i->base != 0 && poly_derived)
      continue;

    data_member& m (*i->member);

    // If the section is soft- added or deleted, check the version.
    //
    unsigned long long av (added (m));
    unsigned long long dv (deleted (m));
    if (av != 0 || dv != 0)
    {
      os << "if (";

      if (av != 0)
        os << "svm >= schema_version_migration (" << av << "ULL, true)";

      if (av != 0 && dv != 0)
        os << " &&" << endl;

      if (dv != 0)
        os << "svm <= schema_version_migration (" << dv << "ULL, true)";

      os << ")" << endl;
    }

    // Section access is always by reference.
    //
    member_access& ma (m.get<member_access> ("get"));
    if (!ma.synthesized)
      os << "// From " << location_string (ma.loc, true) << endl;

    os << ma.translate ("obj") << ".reset (true, false);"
       << endl;
  }

  // Call callback (post_persist).
  //
  if (!abst) // If we are poly-abstract, then top will always be false.
  {
    if (poly)
      os << "if (top)" << endl;

    os << "callback (db," << endl
       << (auto_id ? "static_cast<const object_type&> (obj)," : "obj,") << endl
       << "callback_event::post_persist);";
  }

  os << "}";

  // persist() bulk
  //
  if (c.count ("bulk-persist"))
  {
    os << "void " << traits << "::" << endl
       << "persist (database& db," << endl
       << (auto_id ? "" : "const ") << "object_type** objs," << endl
       << "std::size_t n," << endl
       << "multiple_exceptions& mex)"
       << "{"
       << "using namespace " << db << ";"
       << endl;

    os << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());";

    if (versioned ||
        uss.count (user_sections::count_new |
                   user_sections::count_all |
                   user_sections::count_versioned_only) != 0)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    os << endl;

    os << "for (std::size_t i (0); i != n; ++i)"
       << "{"
       << "const object_type& obj (*objs[i]);"
       << "callback (db, obj, callback_event::pre_persist);"
      //@@ assumption: generate_grow is false or it only affects select (like
      //               in pgsql) so all we have to do is to increment image
      //               version if it grew.
      //@@ assumption: insert_send_auto_id is false
       << "image_type& im (sts.image (i));";

    if (generate_grow)
      os << "if (";

    os << "init (im, obj, statement_insert" << (versioned ? ", svm" : "") << ")";

    if (generate_grow)
      os << " && i == 0)" << endl
         << "im.version++";

    os << ";"
       << "}";

    //@@ assumption: generate_grow: as above
    os << "binding& imb (sts.insert_image_binding ());"
       << "if (imb.version == 0)"
       << "{"
       << "bind (imb.bind, sts.image (), statement_insert" <<
      (versioned ? ", svm" : "") << ");"
       << "imb.version++;"
       << "}";

    // Bind id image since that's where the returned id and/or version will
    // be extracted.
    //
    bool bv (opt != 0 && optimistic_insert_bind_version (*opt));
    if (bv || auto_id)
    {
      os << "binding& idb (sts.id_image_binding ());"
        //@@ assumption: generate_grow: as above
         << "if (idb.version == 0)"
         << "{"
         << "bind (idb.bind, sts.id_image ());"
         << "idb.version++;";
      if (opt != 0)
        os << "sts.optimistic_id_image_binding ().version++;";
      os << "}";
    }

    os << "insert_statement& st (sts.persist_statement ());"
       << "n = st.execute (n, mex);" // Actual number of rows attempted.
       << endl;

    os << "for (std::size_t i (0); i != n; ++i)"
       << "{"
       << "bool r (st.result (i));" // Sets current in mex.
       << endl
       << "if (mex[i] != 0)" << endl // Pending exception for this position.
       << "continue;"
       << endl
       << "if (!r)"
       << "{"
       << "mex.insert (i, object_already_persistent ());"
       << "continue;"
       << "}"
       << "if (mex.fatal ())" << endl // Don't do any extra work.
       << "continue;"
       << endl
       << (auto_id ? "" : "const ") << "object_type& obj (*objs[i]);"
       << endl;

    // Extract auto id.
    //
    if (auto_id) // idb == idf, since auto
    {
      set_member (*idf, "obj", "id (sts.id_image (i))", "db", "id_type");
      os << endl;
    }

    // Set the optimistic concurrency version.
    //
    if (opt != 0)
    {
      // If we don't have auto id, then obj is a const reference.
      //
      set_member (*opt,
                  (auto_id ? "obj" : "const_cast<object_type&> (obj)"),
                  optimistic_version_init (*opt, true),
                  "", // No database.
                  "version_type");
      os << endl;
    }

    // Reset sections: loaded, unchanged.
    //
    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      // Skip special sections.
      //
      if (i->special == user_section::special_version)
        continue;

      data_member& m (*i->member);

      // If the section is soft- added or deleted, check the version.
      //
      unsigned long long av (added (m));
      unsigned long long dv (deleted (m));
      if (av != 0 || dv != 0)
      {
        os << "if (";

        if (av != 0)
          os << "svm >= schema_version_migration (" << av << "ULL, true)";

        if (av != 0 && dv != 0)
          os << " &&" << endl;

        if (dv != 0)
          os << "svm <= schema_version_migration (" << dv << "ULL, true)";

        os << ")" << endl;
        }

      // Section access is always by reference.
      //
      member_access& ma (m.get<member_access> ("get"));
      if (!ma.synthesized)
        os << "// From " << location_string (ma.loc, true) << endl;

      os << ma.translate ("obj") << ".reset (true, false);"
         << endl;
    }

    os << "callback (db," << endl
       << (auto_id ? "static_cast<const object_type&> (obj)," : "obj,") << endl
       << "callback_event::post_persist);"
       << "}"  // for
       << "}"; // persist ()
  }

  // update ()
  //
  if (id != 0 && (!readonly || poly))
  {
    size_t update_containers (
      has_a (c, test_readwrite_container, &main_section));

    bool update_versioned_containers (
      update_containers >
      has_a (c,
             test_readwrite_container |
             exclude_deleted | exclude_added | exclude_versioned,
             &main_section));

    // See if we have any sections that we might have to update.
    //
    bool sections (false);
    bool versioned_sections (false);
    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      // This test will automatically skip the special version update section.
      //
      if (i->update_empty () || i->update == user_section::update_manual)
        continue;

      sections = true;

      if (added (*i->member) || deleted (*i->member))
        versioned_sections = true;

      // For change-updated sections, also check if we have any
      // versioned smart containers.
      //
      if (i->update != user_section::update_change)
        continue;

      if (has_a (c, test_smart_container, &*i) !=
          has_a (c, test_smart_container | exclude_deleted | exclude_added, &*i))
        versioned_sections = true;
    }

    os << "void " << traits << "::" << endl
       << "update (database& db, const object_type& obj";

    if (poly)
      os << ", bool top, bool dyn";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (db);";

    if (poly)
      os << "ODB_POTENTIALLY_UNUSED (top);";

    os << endl
       << "using namespace " << db << ";"
       << "using " << db << "::update_statement;"
       << endl;

    if (poly)
      os << "if (dyn)" << endl
         << "{"
         << "const std::type_info& t (typeid (obj));"
         << endl
         << "if (t != info.type)"
         << "{"
         << "const info_type& pi (root_traits::map->find (t));"
         << "pi.dispatch (info_type::call_update, db, &obj, 0);"
         << "return;"
         << "}"
         << "}";

    // If we are database-poly-abstract but not C++-abstract, then make
    // sure we are not trying to update an instance of an abstract class.
    //
    if (abst && !c.abstract ())
      os << "if (top)" << endl
         << "throw abstract_class ();"
         << endl;

    // If we are readonly, then there is nothing else to do.
    //
    if (!readonly)
    {
      // Call callback (pre_update).
      //
      if (!abst) // If we are poly-abstract then top will always be false.
      {
        if (poly)
          os << "if (top)" << endl;

        os << "callback (db, obj, callback_event::pre_update);"
           << endl;
      }

      bool sts (false);

      if ((versioned && update_columns) ||
          update_versioned_containers ||
          versioned_sections)
      {
        sts = true;
        os << db << "::transaction& tr (" << db <<
          "::transaction::current ());"
           << db << "::connection& conn (tr.connection (db));"
           << "statements_type& sts (" << endl
           << "conn.statement_cache ().find_object<object_type> ());"
           << endl;

        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));"
           << endl;
      }

      // If we have change-updated sections that contain change-tracking
      // containers, then mark such sections as changed if any of the
      // containers were changed. Do this before calling the base so that
      // we account for the whole hierarchy before we actually start
      // updating any sections (important for optimistic concurrency
      // version).
      //
      for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
      {
        user_section& s (*i);

        if (s.update != user_section::update_change)
          continue;

        if (!has_a (c, test_smart_container, &s))
          continue;

        data_member& m (*s.member);

        os << "// " << m.name () << endl
           << "//" << endl;

        // If the section is soft- added or deleted, check the version.
        //
        unsigned long long av (added (m));
        unsigned long long dv (deleted (m));
        if (av != 0 || dv != 0)
        {
          os << "if (";

          if (av != 0)
            os << "svm >= schema_version_migration (" << av << "ULL, true)";

          if (av != 0 && dv != 0)
            os << " &&" << endl;

          if (dv != 0)
            os << "svm <= schema_version_migration (" << dv << "ULL, true)";

          os << ")";
        }

        os << "{";

        // Section access is always by reference.
        //
        member_access& ma (m.get<member_access> ("get"));
        if (!ma.synthesized)
          os << "// From " << location_string (ma.loc, true) << endl;

        // VC++ cannot grok the constructor syntax.
        //
        os << "const odb::section& s = " << ma.translate ("obj") << ";"
           << endl;

        os << "if (";

        // Unless we are always loaded, test for being loaded.
        //
        if (s.load != user_section::load_eager)
          os << "s.loaded () && ";

        // And for not already being changed.
        //
        os << "!s.changed ())"
           << "{";

        instance<container_calls> t (container_calls::section_call, &s);
        t->traverse (c);

        os << "}" // if
           << "}";
      }

      if (poly_derived)
      {
        bool readonly_base (context::readonly (*poly_base));

        if (!sts && (readonly_base ||
                     update_columns ||
                     update_containers ||
                     sections))
        {
          os << db << "::transaction& tr (" << db <<
            "::transaction::current ());"
             << db << "::connection& conn (tr.connection (db));"
             << "statements_type& sts (" << endl
             << "conn.statement_cache ().find_object<object_type> ());"
             << endl;
        }

        // Unless our base is readonly, call it first.
        //
        if (!readonly_base)
        {
          os << "base_traits::update (db, obj, false, false);"
             << endl;
        }
        else
        {
          // Otherwise, we have to initialize the id image ourselves. If
          // we don't have any columns, containers or sections to update,
          // then we only have to do it if this is not a top-level call.
          // If we are abstract, then top is always false.
          //
          if (!update_columns && !update_containers && !sections && !abst)
            os << "if (!top)";

          os << "{"
             << "id_image_type& i (sts.id_image ());"
             << "init (i, id (obj));"
             << endl;

          os << "binding& idb (sts.id_image_binding ());"
             << "if (i.version != sts.id_image_version () || idb.version == 0)"
             << "{"
             << "bind (idb.bind, i);"
             << "sts.id_image_version (i.version);"
             << "idb.version++;"
            // Optimistic poly base cannot be readonly.
             << "}"
             << "}";
        }

        if (update_columns)
        {
          // Initialize the object image.
          //
          os << "image_type& im (sts.image ());";

          if (generate_grow)
            os << "if (";

          os << "init (im, obj, statement_update" <<
            (versioned ? ", svm" : "") << ")";

          if (generate_grow)
            os << ")" << endl
               << "im.version++";

          os << ";"
             << endl;

          os << "const binding& idb (sts.id_image_binding ());"
             << "binding& imb (sts.update_image_binding ());"
             << "if (idb.version != sts.update_id_binding_version () ||" << endl
             << "im.version != sts.update_image_version () ||" << endl
             << "imb.version == 0)"
             << "{"
             << "bind (imb.bind, idb.bind, idb.count, im, statement_update" <<
            (versioned ? ", svm" : "") << ");"
             << "sts.update_id_binding_version (idb.version);"
             << "sts.update_image_version (im.version);"
             << "imb.version++;"
             << "}";

          os << "update_statement& st (sts.update_statement ());"
             << "if (";

          if (versioned)
            os << "!st.empty () && ";

          os << "st.execute () == 0)" << endl
             << "throw object_not_persistent ();"
             << endl;
        }

        // Otherwise, nothing else to do here if we don't have any columns
        // to update.
      }
      else if (update_columns)
      {
        if (!sts)
          os << db << "::transaction& tr (" << db <<
            "::transaction::current ());"
             << db << "::connection& conn (tr.connection (db));"
             << "statements_type& sts (" << endl
             << "conn.statement_cache ().find_object<object_type> ());"
             << endl;

        // Initialize id image.
        //
        if (opt != 0)
          os << "const version_type& v (version (obj));";

        os << "id_image_type& idi (sts.id_image ());"
           << "init (idi, id (obj)" << (opt != 0 ? ", &v" : "") << ");"
           << endl;

        // Initialize object image.
        //
        os << "image_type& im (sts.image ());";

        if (generate_grow)
          os << "if (";

        os << "init (im, obj, statement_update" <<
          (versioned ? ", svm" : "") << ")";

        if (generate_grow)
          os << ")" << endl
             << "im.version++";

        os << ";"
           << endl;

        // The update binding is bound to two images (object and id)
        // so we have to track both versions.
        //
        os << "bool u (false);" // Avoid incrementing version twice.
           << "binding& imb (sts.update_image_binding ());"
           << "if (im.version != sts.update_image_version () ||" << endl
           << "imb.version == 0)"
           << "{"
           << "bind (imb.bind, im, statement_update" <<
          (versioned ? ", svm" : "") << ");"
           << "sts.update_image_version (im.version);"
           << "imb.version++;"
           << "u = true;"
           << "}";

        // To update the id part of the update binding we have to do
        // it indirectly via the id binding, which just points to the
        // suffix of the update bind array (see object_statements).
        //
        os << "binding& idb (sts.id_image_binding ());"
           << "if (idi.version != sts.update_id_image_version () ||" << endl
           << "idb.version == 0)"
           << "{"
          // If the id binding is up-to-date, then that means update
          // binding is too and we just need to update the versions.
          //
           << "if (idi.version != sts.id_image_version () ||" << endl
           << "idb.version == 0)"
           << "{"
           << "bind (idb.bind, idi);"
          // Update the id binding versions since we may use them later
          // to update containers.
          //
           << "sts.id_image_version (idi.version);"
           << "idb.version++;";
        if (opt != 0)
          os << "sts.optimistic_id_image_binding ().version++;";
        os << "}"
           << "sts.update_id_image_version (idi.version);"
           << endl
           << "if (!u)" << endl
           << "imb.version++;"
           << "}";

        os << "update_statement& st (sts.update_statement ());"
           << "if (";

        if (versioned)
          os << "!st.empty () && ";

        os << "st.execute () == 0)" << endl;

        if (opt == 0)
          os << "throw object_not_persistent ();";
        else
          os << "throw object_changed ();";

        os << endl;
      }
      else if (poly || update_containers || sections)
      {
        // We don't have any columns to update but we may still need
        // to initialize the id image if we are polymorphic or have
        // any containers or sections.
        //
        if (!sts)
          os << db << "::transaction& tr (" << db <<
            "::transaction::current ());"
             << db << "::connection& conn (tr.connection (db));"
             << "statements_type& sts (" << endl
             << "conn.statement_cache ().find_object<object_type> ());"
             << endl;

        // The same rationale as a couple of screens up.
        //
        if (poly && !update_containers && !sections && !abst)
          os << "if (!top)";

        os << "{"
           << "id_image_type& i (sts.id_image ());"
           << "init (i, id (obj));"
           << endl;

        os << "binding& idb (sts.id_image_binding ());"
           << "if (i.version != sts.id_image_version () || idb.version == 0)"
           << "{"
           << "bind (idb.bind, i);"
           << "sts.id_image_version (i.version);"
           << "idb.version++;"
          // Cannot be optimistic.
           << "}"
           << "}";
      }

      if (update_containers || sections)
        os << "extra_statement_cache_type& esc (sts.extra_statement_cache ());"
           << endl;

      if (update_containers)
      {
        instance<container_calls> t (container_calls::update_call,
                                     &main_section);
        t->traverse (c);
      }

      // Update the optimistic concurrency version in the object member.
      // Do it before updating sections since they will update it as well.
      // The same code as in section_traits.
      //
      if (opt != 0 && !poly_derived)
      {
        // Object is passed as const reference so we need to cast away
        // constness.
        //
        const char* obj ("const_cast<object_type&> (obj)");
        string inc (optimistic_version_increment (*opt));

        if (inc == "1")
          inc_member (*opt, obj, "obj", "version_type");
        else
          set_member (*opt, obj, inc, "", "version_type");

        os << endl;
      }

      // Update sections that are loaded and need updating.
      //
      for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
      {
        if (i->update_empty () || i->update == user_section::update_manual)
          continue;

        // Here is the deal: for polymorphic section overrides, we could
        // have used the same approach as in load_() where the class that
        // defines the section data member initiates the section update.
        // However, if we used this approach, then we would get what can
        // be called "interleaved" update statements. That is, a section
        // update would update the section data in "derived" tables before
        // updating the main section in these derived tables. While this
        // does not feel right, it can also cause more real problems if,
        // for example, there are constraints or some such. Generally,
        // we always want to update the main section data first for each
        // table in the hierarchy.
        //
        // So what we are going to do instead is call section traits
        // update at each override point (and indicate to it not to
        // call base). One tricky aspect is who is going to reset the
        // changed state. We have to do it at the top since otherwise
        // overrides won't know the section has changed. Finding out
        // whether we are the final override (in section terms, not
        // object terms) is tricky.
        //
        data_member& m (*i->member);

        // If the section is soft- added or deleted, check the version.
        //
        unsigned long long av (added (m));
        unsigned long long dv (deleted (m));
        if (av != 0 || dv != 0)
        {
          os << "if (";

          if (av != 0)
            os << "svm >= schema_version_migration (" << av << "ULL, true)";

          if (av != 0 && dv != 0)
            os << " &&" << endl;

          if (dv != 0)
            os << "svm <= schema_version_migration (" << dv << "ULL, true)";

          os << ")"
             << "{";
        }

        // Section access is always by reference.
        //
        member_access& ma (m.get<member_access> ("get"));
        if (!ma.synthesized)
          os << "// From " << location_string (ma.loc, true) << endl;

        os << "if (";

        // Unless we are always loaded, test for being loaded.
        //
        string sep;
        if (i->load != user_section::load_eager)
        {
          os << ma.translate ("obj") << ".loaded ()";
          sep = " && ";
        }

        // If change-updated section, check that it has changed.
        //
        if (i->update == user_section::update_change)
          os << sep << ma.translate ("obj") << ".changed ()";

        os << ")"
           << "{";

        // Re-initialize the id+ver image with new version which may
        // have changed. Here we take a bit of a shortcut and not
        // re-bind the image since we know only version may have
        // changed and that is always an integer (cannot grow).
        //
        if (opt != 0)
        {
          os << "const version_type& v (version (obj));"
             << "init (sts.id_image (), id (obj), &v);"
             << endl;
        }

        os << public_name (m) << "_traits::update (esc, obj";

        if (poly_derived && i->base != 0)
        {
          // Normally we don't want to call base (base object's update()
          // would have called it; see comment above). There is one
          // exception, however, and that is a readonly base section
          // in optimistic class. In this case, base object's update()
          // won't call it (it is readonly) but it needs to be called
          // in order to increment the version.
          //
          bool base (false);
          for (user_section* b (i->base); !base && b != 0; b = b->base)
          {
            if (b->total != b->inverse + b->readonly ||
                b->readwrite_containers)
              break; // We have a readwrite base.
            else if (b->optimistic ())
              base = true; // We have a readonly optimistic root.
          }

          os << ", " << base;
        }

        os << ");";

        // Clear the change flag and arm the transaction rollback callback.
        //
        if (i->update == user_section::update_change)
        {
          // If polymorphic, do it only if we are the final override.
          //
          if (poly)
            os << "if (root_traits::map->find (typeid (obj))." <<
              "final_section_update (info, " << i->index << "UL))" << endl;

          os << ma.translate ("obj") << ".reset (true, false, &tr);";
        }

        os << "}";

        if (av != 0 || dv != 0)
          os << "}";
      }

      // Call callback (post_update).
      //
      if (!abst) // If we are poly-abstract, then top will always be false.
      {
        if (poly)
          os << "if (top)"
             << "{";

        os << "callback (db, obj, callback_event::post_update);"
           << "pointer_cache_traits::update (db, obj);";

        if (poly)
          os << "}";
      }
    } // readonly

    os << "}";
  }

  // update () bulk
  //
  if (id != 0 && !readonly && c.count ("bulk-update"))
  {
    os << "void " << traits << "::" << endl
       << "update (database& db," << endl
       << "const object_type** objs," << endl
       << "std::size_t n," << endl
       << "multiple_exceptions& mex)"
       << "{"
       << "using namespace " << db << ";"
       << "using " << db << "::update_statement;"
       << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());";

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    os << endl
       << "for (std::size_t i (0); i != n; ++i)"
       << "{"
       << "const object_type& obj (*objs[i]);"
       << "callback (db, obj, callback_event::pre_update);";

    if (opt != 0)
      os << "const version_type& v (version (obj));";

    os << "init (sts.id_image (i), id (obj)" << (opt != 0 ? ", &v" : "") << ");";

    //@@ assumption: generate_grow is false or it only affects select (like
    //               in pgsql) so all we have to do is to increment image
    //               version if it grew.

    os << "image_type& im (sts.image (i));";

    if (generate_grow)
      os << "if (";

    os << "init (im, obj, statement_update" << (versioned ? ", svm" : "") << ")";

    if (generate_grow)
      os << " && i == 0)" << endl
         << "im.version++";

    os << ";"
       << "}";

    // Update bindings.
    //
    os << "binding& idb (sts.id_image_binding ());"
       << "binding& imb (sts.update_image_binding ());"
       << endl;

    //@@ assumption: generate_grow: as above
    //
    os << "bool u (false);" // Avoid incrementing version twice.
       << "if (imb.version == 0)"
       << "{"
       << "bind (imb.bind, sts.image (), statement_update" <<
      (versioned ? ", svm" : "") << ");"
       << "imb.version++;"
       << "u = true;"
       << "}";

    //@@ assumption: generate_grow: as above
    //
    os << "if (idb.version == 0)"
       << "{"
       << "bind (idb.bind, sts.id_image ());"
       << "idb.version++;";
    if (opt != 0)
      os << "sts.optimistic_id_image_binding ().version++;";
    os << endl
       << "if (!u)" << endl
       << "imb.version++;"
       << "}";

    // We don't need the versioned check here since the statement cannot
    // be empty; otherwise it would have been an empty object which is
    // illegal.
    //
    os << "update_statement& st (sts.update_statement ());"
       << "n = st.execute (n, mex);"; // Actual number of rows attempted.

    os << endl
       << "for (std::size_t i (0); i != n; ++i)"
       << "{"
       << "unsigned long long r (st.result (i));" // Also sets current in mex.
       << endl
       << "if (mex[i] != 0)" << endl // Pending exception from result().
       << "continue;"
       << endl
       << "if (r != 1)"
       << "{"
       << "mex.insert (i," << endl
      //@@ assumption: result_unknown
       << "(r == update_statement::result_unknown)," << endl
       << (opt == 0 ? "object_not_persistent" : "object_changed") << " ());"
       << "continue;"
       << "}"
       << "if (mex.fatal ())" << endl // Don't do any extra work.
       << "continue;"
       << endl
       << "const object_type& obj (*objs[i]);";

    // Update the optimistic concurrency version in the object member.
    //
    if (opt != 0)
    {
      // Object is passed as const reference so we need to cast away
      // constness.
      //
      const char* obj ("const_cast<object_type&> (obj)");
      string inc (optimistic_version_increment (*opt));

      if (inc == "1")
        inc_member (*opt, obj, "obj", "version_type");
      else
        set_member (*opt, obj, inc, "", "version_type");

      os << endl;
    }

    os << "callback (db, obj, callback_event::post_update);"
       << "pointer_cache_traits::update (db, obj);"
       << "}"  // for
       << "}"; // update()
  }

  // erase (id)
  //
  size_t erase_containers (has_a (c, test_straight_container));
  bool erase_versioned_containers (
    erase_containers >
    has_a (c, test_straight_container | exclude_deleted | exclude_added));

  if (id != 0)
  {
    os << "void " << traits << "::" << endl
       << "erase (database& db, const id_type& id";

    if (poly)
      os << ", bool top, bool dyn";

    os << ")"
       << "{"
       << "using namespace " << db << ";";

    if (poly)
      os << endl
         << "ODB_POTENTIALLY_UNUSED (top);";

    os << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());"
       << endl;

    // Get the discriminator and determine the dynamic type of
    // this object.
    //
    if (poly)
    {
      os << "if (dyn)" << endl
         << "{"
         << "discriminator_type d;"
         << "root_traits::discriminator_ (sts.root_statements (), id, &d);";

      if (!abst)
        os << endl
           << "if (d != info.discriminator)"
           << "{";

      os << "const info_type& pi (root_traits::map->find (d));"
         << endl
        // Check that the dynamic type is derived from the static type.
        //
         << "if (!pi.derived (info))" << endl
         << "throw object_not_persistent ();"
         << endl
         << "pi.dispatch (info_type::call_erase, db, 0, &id);"
         << "return;";

      if (!abst)
        os << "}";

      os << "}";
    }

    // Initialize id image.
    //
    if (!abst) // If we are poly-abstract, then top will always be false.
    {
      if (poly)
        os << "if (top)"
           << "{";

      os << "id_image_type& i (sts.id_image ());"
         << "init (i, id);"
         << endl;

      os << "binding& idb (sts.id_image_binding ());"
         << "if (i.version != sts.id_image_version () || idb.version == 0)"
         << "{"
         << "bind (idb.bind, i);"
         << "sts.id_image_version (i.version);"
         << "idb.version++;";
      if (opt != 0)
        os << "sts.optimistic_id_image_binding ().version++;";
      os << "}";

      if (poly)
        os << "}";
    }

    // Erase containers first so that there are no reference
    // violations (we don't want to rely on ON DELETE CASCADE
    // here since in case of a custom schema, it might not be
    // there).
    //
    if (erase_containers)
    {
      os << "extra_statement_cache_type& esc (sts.extra_statement_cache ());";

      if (erase_versioned_containers)
        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));";

      os << endl;

      instance<container_calls> t (container_calls::erase_id_call);
      t->traverse (c);
    }

    os << "if (sts.erase_statement ().execute () != 1)" << endl
       << "throw object_not_persistent ();"
       << endl;

    if (poly_derived)
    {
      // Call our base last (we erase polymorphic objects from base
      // to derived in order not to trigger cascading deletes).
      //
      os << "base_traits::erase (db, id, false, false);"
         << endl;
    }

    // Remove from the object cache.
    //
    if (!abst) // If we are poly-abstract, then top will always be false.
    {
      if (poly)
        os << "if (top)" << endl;

      os << "pointer_cache_traits::erase (db, id);";
    }

    os << "}";
  }

  // erase (id) bulk
  //
  if (id != 0 && c.count ("bulk-erase"))
  {
    os << "std::size_t " << traits << "::" << endl
       << "erase (database& db," << endl
       << "const id_type** ids," << endl
       << "std::size_t n," << endl
       << "multiple_exceptions& mex)"
       << "{"
       << "using namespace " << db << ";"
       << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());"
       << endl
       << "for (std::size_t i (0); i != n; ++i)" << endl
       << "init (sts.id_image (i), *ids[i]);"
       << endl
       << "binding& idb (sts.id_image_binding ());"
      //@@ assumption: generate_grow is false or it only affects select (like
      //               in pgsql).
       << "if (idb.version == 0)"
       << "{"
       << "bind (idb.bind, sts.id_image ());"
       << "idb.version++;"
       << (opt != 0 ? "sts.optimistic_id_image_binding ().version++;" : "")
       << "}"
       << "delete_statement& st (sts.erase_statement ());"
       << "n = st.execute (n, mex);" // Set to actual number of rows attempted.
       << endl
       << "for (std::size_t i (0); i != n; ++i)"
       << "{"
       << "unsigned long long r (st.result (i));" // Sets current in mex.
       << endl
       << "if (mex[i] != 0)" << endl // Pending exception from result().
       << "continue;"
       << endl
       << "if (r != 1)"
       << "{"
       << "mex.insert (i," << endl
      //@@ assumption: result_unknown
       << "(r == delete_statement::result_unknown)," << endl
       << "object_not_persistent ());"
       << "continue;"
       << "}"
       << "if (mex.fatal ())" << endl // Don't do any extra work.
       << "continue;"
       << "pointer_cache_traits::erase (db, *ids[i]);"
       << "}"  // for
       << "return n;"
       << "}"; // erase()
  }

  // erase (object)
  //
  bool erase_smart_containers (has_a (c, test_smart_container));

  if (id != 0 && (poly || opt != 0 || erase_smart_containers))
  {
    os << "void " << traits << "::" << endl
       << "erase (database& db, const object_type& obj";

    if (poly)
      os << ", bool top, bool dyn";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (db);";

    if (poly)
      os << "ODB_POTENTIALLY_UNUSED (top);";

    os << endl;

    if (poly)
      os << "if (dyn)" << endl
         << "{"
         << "const std::type_info& t (typeid (obj));"
         << endl
         << "if (t != info.type)"
         << "{"
         << "const info_type& pi (root_traits::map->find (t));"
         << "pi.dispatch (info_type::call_erase, db, &obj, 0);"
         << "return;"
         << "}"
         << "}";

    // If we are database-poly-abstract but not C++-abstract, then make
    // sure we are not trying to erase an instance of an abstract class.
    //
    if (abst && !c.abstract ())
      os << "if (top)" << endl
         << "throw abstract_class ();"
         << endl;

    if (opt != 0 || erase_smart_containers)
    {
      string rsts (poly_derived ? "rsts" : "sts");

      os << "using namespace " << db << ";"
         << endl
         << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection (db));"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (poly_derived)
        os << endl
           << "root_statements_type& rsts (sts.root_statements ());"
           << "ODB_POTENTIALLY_UNUSED (rsts);";

      os << endl;

      // Call callback (pre_erase).
      //
      if (!abst) // If we are poly-abstract, then top will always be false.
      {
        if (poly)
          os << "if (top)" << endl;

        os << "callback (db, obj, callback_event::pre_erase);"
           << endl;
      }

      if (!abst || erase_containers)
      {
        os << "const id_type& id (object_traits_impl::id (obj));"
           << endl;
      }

      // Smart containers case.
      //
      if (opt == 0)
      {
        // Initialize id image.
        //
        if (!abst) // If we are poly-abstract, then top will always be false.
        {
          if (poly)
            os << "if (top)"
               << "{";

          os << "id_image_type& i (" << rsts << ".id_image ());"
             << "init (i, id);"
             << endl;

          os << "binding& idb (" << rsts << ".id_image_binding ());"
             << "if (i.version != " << rsts << ".id_image_version () || " <<
            "idb.version == 0)"
             << "{"
             << "bind (idb.bind, i);"
             << rsts << ".id_image_version (i.version);"
             << "idb.version++;"
            // Cannot be optimistic.
             << "}";

          if (poly)
            os << "}";
        }

        // Erase containers first so that there are no reference
        // violations (we don't want to rely on ON DELETE CASCADE
        // here since in case of a custom schema, it might not be
        // there).
        //

        if (erase_containers)
        {
          os << "extra_statement_cache_type& esc (" <<
            "sts.extra_statement_cache ());";

          if (erase_versioned_containers)
            os << "const schema_version_migration& svm (" <<
              "sts.version_migration (" << schema_name << "));";

          os << endl;

          instance<container_calls> t (container_calls::erase_obj_call);
          t->traverse (c);
        }

        os << "if (sts.erase_statement ().execute () != 1)" << endl
           << "throw object_not_persistent ();"
           << endl;
      }
      // Optimistic case.
      //
      else
      {
        // Initialize id + managed column image.
        //
        if (!abst) // If we are poly-abstract, then top will always be false.
        {
          if (poly)
            os << "if (top)"
               << "{";

          os << "const version_type& v (version (obj));"
             << "id_image_type& i (" << rsts << ".id_image ());"
             << "init (i, id, &v);"
             << endl;

          os << "binding& idb (" << rsts << ".id_image_binding ());"
             << "if (i.version != " << rsts << ".id_image_version () ||" << endl
             << "idb.version == 0)"
             << "{"
             << "bind (idb.bind, i);"
             << rsts << ".id_image_version (i.version);"
             << "idb.version++;"
             << rsts << ".optimistic_id_image_binding ().version++;"
             << "}";

          if (poly)
            os << "}"; // if (top)
        }

        // If this is a derived type in a polymorphic hierarchy, then we
        // need to check the version (stored in root) before we go ahead
        // and start deleting things. Also use the same code for root with
        // containers since it is more efficient than the find_() method
        // below.
        //
        bool svm (false);
        if (poly_derived || (poly && erase_containers))
        {
          // Only do the check in the top-level call.
          //
          if (!abst) // If we are poly-abstract, then top will always be false.
          {
            os << "if (top)"
               << "{"
               << "version_type v;"
               << "root_traits::discriminator_ (" << rsts << ", id, 0, &v);"
               << endl;

            os << "if (v != version (obj))" << endl
               << "throw object_changed ();"
               << "}";
          }
        }
        else if (erase_containers)
        {
          // Things get complicated here: we don't want to trash the
          // containers and then find out that the versions don't match
          // and we therefore cannot delete the object. After all, there
          // is no guarantee that the user will abort the transaction.
          // In fact, a perfectly reasonable scenario is to reload the
          // object, re-check the condition, decide not to delete the
          // object, and then commit the transaction.
          //
          // There doesn't seem to be anything better than first making
          // sure we can delete the object, then deleting the container
          // data, and then deleting the object. To check that we can
          // delete the object we are going to use find_() and then
          // compare the versions. A special-purpose SELECT query would
          // have been more efficient but it would complicated and bloat
          // things significantly.
          //

          if (versioned)
          {
            os << "const schema_version_migration& svm (" <<
              "sts.version_migration (" << schema_name << "));"
               << endl;
            svm = true;
          }

          os << "if (!find_ (sts, &id" <<
            (versioned ? ", svm" : "") << "))" << endl
             << "throw object_changed ();"
             << endl;

          if (delay_freeing_statement_result)
            os << "sts.find_statement ().free_result ();"
               << endl;

          os << "if (version (sts.image ()) != version (obj))" << endl
             << "throw object_changed ();"
             << endl;
        }

        // Erase containers first so that there are no reference
        // violations (we don't want to rely on ON DELETE CASCADE
        // here since in case of a custom schema, it might not be
        // there).
        //
        if (erase_containers)
        {
          os << "extra_statement_cache_type& esc (" <<
            "sts.extra_statement_cache ());";

          if (erase_versioned_containers && !svm)
            os << "const schema_version_migration& svm (" <<
              "sts.version_migration (" << schema_name << "));";

          os << endl;

          instance<container_calls> t (container_calls::erase_obj_call);
          t->traverse (c);
        }

        const char* st (
          poly_derived ? "erase_statement" : "optimistic_erase_statement");

        os << "if (sts." << st << " ().execute () != 1)" << endl
           << "throw object_changed ();"
           << endl;
      }

      if (poly_derived)
      {
        // Call our base last (we erase polymorphic objects from base
        // to derived in order not to trigger cascading deletes).
        //
        os << "base_traits::erase (db, obj, false, false);"
           << endl;
      }

      if (!abst) // If we are poly-abstract, then top will always be false.
      {
        if (poly)
          os << "if (top)"
             << "{";

        // Note that we don't reset sections since the object is now
        // transient and the state of a section in a transient object
        // is undefined.

        // Remove from the object cache.
        //
        os << "pointer_cache_traits::erase (db, id);";

        // Call callback (post_erase).
        //
        os << "callback (db, obj, callback_event::post_erase);";

        if (poly)
          os << "}";
      }
    }
    else
    {
      os << "callback (db, obj, callback_event::pre_erase);"
         << "erase (db, id (obj), true, false);"
         << "callback (db, obj, callback_event::post_erase);";
    }

    os << "}";
  }

  // erase (object) bulk
  //
  if (id != 0 && c.count ("bulk-erase"))
  {
    os << "void " << traits << "::" << endl
       << "erase (database& db," << endl
       << "const object_type** objs," << endl
       << "std::size_t n," << endl
       << "multiple_exceptions& mex)"
       << "{";

    // In non-optimistic case delegate to erase(id).
    //
    if (opt == 0)
    {
      os << "id_type a[batch];"
         << "const id_type* p[batch];"
         << endl
         << "for (std::size_t i (0); i != n; ++i)"
         << "{"
         << "const object_type& obj (*objs[i]);"
         << "callback (db, obj, callback_event::pre_erase);"
         << "a[i] = id (obj);"
         << "p[i] = a + i;"
         << "}"
         << "n = erase (db, p, n, mex);"
         << endl
         << "if (mex.fatal ())" << endl // Don't do any extra work.
         << "return;"
         << endl
         << "for (std::size_t i (0); i != n; ++i)"
         << "{"
         << "if (mex[i] == 0)" << endl // No pending exception.
         << "callback (db, *objs[i], callback_event::post_erase);"
         << "}"; // for
    }
    else
    {
      os << "using namespace " << db << ";"
         << endl
         << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection (db));"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());"
         << endl
         << "for (std::size_t i (0); i != n; ++i)"
         << "{"
         << "const object_type& obj (*objs[i]);"
         << "callback (db, obj, callback_event::pre_erase);"
         << "const version_type& v (version (obj));"
         << "init (sts.id_image (i), id (obj), &v);"
         << "}";

      os << "binding& idb (sts.id_image_binding ());"
        //@@ assumption: generate_grow is false or it only affects select
        //               (like in pgsql).
         << "if (idb.version == 0)"
         << "{"
         << "bind (idb.bind, sts.id_image ());"
         << "idb.version++;"
         << "sts.optimistic_id_image_binding ().version++;"
         << "}"
         << "delete_statement& st (sts.optimistic_erase_statement ());"
         << "n = st.execute (n, mex);" // Set to actual number of attempted.
         << endl
         << "for (std::size_t i (0); i != n; ++i)"
         << "{"
         << "unsigned long long r (st.result (i));" // Sets current in mex.
         << endl
         << "if (mex[i] != 0)" << endl // Pending exception from result().
         << "continue;"
         << endl
         << "if (r != 1)"
         << "{"
         << "mex.insert (i," << endl
        //@@ assumption: result_unknown
         << "(r == delete_statement::result_unknown)," << endl
         << "object_changed ());"
         << "continue;"
         << "}"
         << "if (mex.fatal ())" << endl // Don't do any extra work.
         << "continue;"
         << endl
         << "const object_type& obj (*objs[i]);"
         << "pointer_cache_traits::erase (db, id (obj));"
         << "callback (db, obj, callback_event::post_erase);"
         << "}"; // for
    }

    os << "}"; // erase()
  }

  // find (id)
  //
  if (id != 0 && c.default_ctor ())
  {
    string rsts (poly_derived ? "rsts" : "sts");

    os << traits << "::pointer_type" << endl
       << traits << "::" << endl
       << "find (database& db, const id_type& id)"
       << "{"
       << "using namespace " << db << ";"
       << endl;

    // First check the session.
    //
    os << "{";

    if (poly_derived)
      os << "root_traits::pointer_type rp (pointer_cache_traits::find (" <<
        "db, id));"
         << endl
         << "if (!root_traits::pointer_traits::null_ptr (rp))" << endl
         << "return" << endl
         << "  root_traits::pointer_traits::dynamic_pointer_cast<" <<
        "object_type> (rp);";
    else
      os << "pointer_type p (pointer_cache_traits::find (db, id));"
         << endl
         << "if (!pointer_traits::null_ptr (p))" << endl
         << "return p;";

    os << "}";

    os << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());";

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    if (poly_derived)
      os << "root_statements_type& rsts (sts.root_statements ());";

    os << endl
       << "statements_type::auto_lock l (" << rsts << ");";

    if (delay_freeing_statement_result)
      os << "auto_result ar;";

    if (poly)
      os << "root_traits::discriminator_type d;";

    os << endl
       << "if (l.locked ())"
       << "{"
       << "if (!find_ (sts, &id" << (versioned ? ", svm" : "") << "))" << endl
       << "return pointer_type ();";

    if (delay_freeing_statement_result)
      os << endl
         << "ar.set (sts.find_statement (" << (poly_derived ? "depth" : "") <<
        "));";

    if (poly)
      os << "d = root_traits::discriminator (" << rsts << ".image ());";

    os << "}";

    if (poly)
    {
      // If statements are locked, then get the discriminator by
      // executing a special SELECT statement. We need it to be
      // able to create an object of the correct dynamic type
      // which will be loaded later.
      //
      os << "else" << endl
         << "root_traits::discriminator_ (" << rsts << ", id, &d);"
         << endl;

      if (abst)
        os << "const info_type& pi (root_traits::map->find (d));"
           << endl;
      else
        os << "const info_type& pi (" << endl
           << "d == info.discriminator ? info : root_traits::map->find (d));"
           << endl;
    }

    // Create the object.
    //
    if (poly_derived)
    {
      os << "root_traits::pointer_type rp (pi.create ());"
         << "pointer_type p (" << endl
         << "root_traits::pointer_traits::static_pointer_cast<object_type> " <<
        "(rp));"
         << "pointer_traits::guard pg (p);"
         << endl;

      // Insert it as a root pointer (for non-unique pointers, rp should
      // still be valid and for unique pointers this is a no-op).
      //
      os << "pointer_cache_traits::insert_guard ig (" << endl
         << "pointer_cache_traits::insert (db, id, rp));"
         << endl;
    }
    else
    {
      if (poly)
        os << "pointer_type p (pi.create ());";
      else
        os << "pointer_type p (" << endl
           << "access::object_factory<object_type, pointer_type>::create ());";

      os << "pointer_traits::guard pg (p);"
         << endl;

      os << "pointer_cache_traits::insert_guard ig (" << endl
         << "pointer_cache_traits::insert (db, id, p));"
         << endl;
    }

    os << "object_type& obj (pointer_traits::get_ref (p));"
       << endl
       << "if (l.locked ())"
       << "{"
       << "select_statement& st (sts.find_statement (" <<
      (poly_derived ? "depth" : "") << "));"
       << "ODB_POTENTIALLY_UNUSED (st);"
       << endl;

    if (poly)
      os << "callback_event ce (callback_event::pre_load);"
         << "pi.dispatch (info_type::call_callback, db, &obj, &ce);";
    else
      os << "callback (db, obj, callback_event::pre_load);";

    os << "init (obj, sts.image (), &db" << (versioned ? ", svm" : "") << ");";

    init_value_extra ();

    if (delay_freeing_statement_result)
      os << "ar.free ();";

    os << "load_ (sts, obj, false" << (versioned ? ", svm" : "") << ");";

    if (poly)
      // Load the dynamic part of the object unless static and dynamic
      // types are the same.
      //
      os << endl
         << "if (&pi != &info)"
         << "{"
         << "std::size_t d (depth);"
         << "pi.dispatch (info_type::call_load, db, &obj, &d);"
         << "}";

    os << rsts << ".load_delayed (" << (versioned ? "&svm" : "0") << ");"
       << "l.unlock ();";

    if (poly)
      os << "ce = callback_event::post_load;"
         << "pi.dispatch (info_type::call_callback, db, &obj, &ce);";
    else
      os << "callback (db, obj, callback_event::post_load);";

    os << "pointer_cache_traits::load (ig.position ());"
       << "}"
       << "else" << endl
       << rsts << ".delay_load (id, obj, ig.position ()" <<
      (poly ? ", pi.delayed_loader" : "") << ");"
       << endl;

    os << "ig.release ();"
       << "pg.release ();"
       << "return p;"
       << "}";
  }

  // find (id, obj)
  //
  if (id != 0)
  {
    string rsts (poly_derived ? "rsts" : "sts");

    os << "bool " << traits << "::" << endl
       << "find (database& db, const id_type& id, object_type& obj";

    if (poly)
      os << ", bool dyn";

    os << ")"
       << "{";

    if (poly)
      os << "ODB_POTENTIALLY_UNUSED (dyn);"
         << endl;

    os << "using namespace " << db << ";"
       << endl;

    if (poly)
    {
      if (!abst)
      os << "if (dyn)" << endl
         << "{";

      os << "const std::type_info& t (typeid (obj));";

      if (!abst)
        os << endl
           << "if (t != info.type)"
           << "{";

      os << "const info_type& pi (root_traits::map->find (t));"
         << "return pi.dispatch (info_type::call_find, db, &obj, &id);";

      if (!abst)
        os << "}"
           << "}";
    }

    if (!abst)
    {
      os << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection (db));"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (versioned)
        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));";

      if (poly_derived)
        os << "root_statements_type& rsts (sts.root_statements ());";

      // This can only be top-level call so auto_lock must succeed.
      //
      os << endl
         << "statements_type::auto_lock l (" << rsts << ");"
         << "assert (l.locked ()) /* Must be a top-level call. */;"
         << endl;

      os << "if (!find_ (sts, &id" <<
        (versioned ? ", svm" : "") << "))" << endl
         << "return false;"
         << endl;

      os << "select_statement& st (sts.find_statement (" <<
        (poly_derived ? "depth" : "") << "));"
         << "ODB_POTENTIALLY_UNUSED (st);"
         << endl;

      if (delay_freeing_statement_result)
        os << "auto_result ar (st);";

      os << "reference_cache_traits::position_type pos (" << endl
         << "reference_cache_traits::insert (db, id, obj));"
         << "reference_cache_traits::insert_guard ig (pos);"
         << endl
         << "callback (db, obj, callback_event::pre_load);"
         << "init (obj, sts.image (), &db" <<
        (versioned ? ", svm" : "") << ");";

      init_value_extra ();

      if (delay_freeing_statement_result)
        os << "ar.free ();";

      os << "load_ (sts, obj, false" << (versioned ? ", svm" : "") << ");"
         << rsts << ".load_delayed (" << (versioned ? "&svm" : "0") << ");"
         << "l.unlock ();"
         << "callback (db, obj, callback_event::post_load);"
         << "reference_cache_traits::load (pos);"
         << "ig.release ();"
         << "return true;";
    }

    os << "}";
  }

  // reload ()
  //
  if (id != 0)
  {
    string rsts (poly_derived ? "rsts" : "sts");

    // This implementation is almost exactly the same as find(id, obj)
    // above except that it doesn't interract with the object cache and,
    // in case of optimistic concurrency, checks if the object actually
    // needs to be reloaded.
    //
    os << "bool " << traits << "::" << endl
       << "reload (database& db, object_type& obj";

    if (poly)
      os << ", bool dyn";

    os << ")"
       << "{";

    if (poly)
      os << "ODB_POTENTIALLY_UNUSED (dyn);"
         << endl;

    os << "using namespace " << db << ";"
       << endl;

    if (poly)
    {
      if (!abst)
      os << "if (dyn)" << endl
         << "{";

      os << "const std::type_info& t (typeid (obj));";

      if (!abst)
        os << endl
           << "if (t != info.type)"
           << "{";

      os << "const info_type& pi (root_traits::map->find (t));"
         << "return pi.dispatch (info_type::call_reload, db, &obj, 0);";

      if (!abst)
        os << "}"
           << "}";
    }

    if (!abst)
    {
      os << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection (db));"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (versioned)
        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));";

      if (poly_derived)
        os << "root_statements_type& rsts (sts.root_statements ());";

      // This can only be top-level call so auto_lock must succeed.
      //
      os << endl
         << "statements_type::auto_lock l (" << rsts << ");"
         << "assert (l.locked ()) /* Must be a top-level call. */;"
         << endl;

      os << "const id_type& id (object_traits_impl::id (obj));"
         << "if (!find_ (sts, &id" <<
        (versioned ? ", svm" : "") << "))" << endl
         << "return false;"
         << endl;

      os << "select_statement& st (sts.find_statement (" <<
        (poly_derived ? "depth" : "") << "));"
         << "ODB_POTENTIALLY_UNUSED (st);"
         << endl;

      if (delay_freeing_statement_result)
        os << "auto_result ar (st);"
           << endl;

      if (opt != 0)
      {
        const char* tr (poly_derived ? "root_traits::" : "");

        os << "if (" << tr << "version (" << rsts << ".image ()) == " <<
          tr << "version (obj))" << endl
           << "return true;"
           << endl;
      }

      os << "callback (db, obj, callback_event::pre_load);"
         << "init (obj, sts.image (), &db" <<
        (versioned ? ", svm" : "") << ");";

      init_value_extra ();

      if (delay_freeing_statement_result)
        os << "ar.free ();";

      os << "load_ (sts, obj, true" << (versioned ? ", svm" : "") << ");"
         << rsts << ".load_delayed (" << (versioned ? "&svm" : "0") << ");"
         << "l.unlock ();"
         << "callback (db, obj, callback_event::post_load);"
         << "return true;";
    }

    os << "}";
  }

  // load (section) [non-thunk version]
  //
  if (uss.count (user_sections::count_new   |
                 user_sections::count_load  |
                 (poly ? user_sections::count_load_empty : 0)) != 0)
  {
    string rsts (poly_derived ? "rsts" : "sts");

    os << "bool " << traits << "::" << endl
       << "load (connection& conn, object_type& obj, section& s" <<
      (poly ? ", const info_type* pi" : "") << ")"
       << "{"
       << "using namespace " << db << ";"
       << endl;

    if (poly)
    {
      // Resolve type information if we are doing indirect calls.
      //
      os << "bool top (pi == 0);" // Top-level call.
         << "if (top)"
         << "{"
         << "const std::type_info& t (typeid (obj));";

      if (!abst)
        os << endl
           << "if (t == info.type)" << endl
           << "pi = &info;"
           << "else" << endl;

      os << "pi = &root_traits::map->find (t);"
         << "}";
    }

    // Lock the statements for the object itself. We need to do this since we
    // are using the id image and loading of object pointers can overwrite it.
    //
    os << db << "::connection& c (static_cast<" << db <<
      "::connection&> (conn));"
       << "statements_type& sts (c.statement_cache ()." <<
      "find_object<object_type> ());";

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    if (poly_derived)
      os << "root_statements_type& rsts (sts.root_statements ());";

    os << endl
       << "statements_type::auto_lock l (" << rsts << ");";

    // It this is a top-level call, then auto_lock must succeed.
    //
    if (poly)
      os << "if (top)" << endl;

    os << "assert (l.locked ()) /* Must be a top-level call. */;"
       << endl;

    os << "bool r (false);"
       << endl;

    // If our poly-base has load sections, then call the base version
    // first. Besides checking for sections, it will also initialize
    // the id image.
    //
    if (poly_derived &&
        buss->count (user_sections::count_total |
                     user_sections::count_load  |
                     user_sections::count_load_empty) != 0)
    {
      os << "if (base_traits::load (conn, obj, s, pi))" << endl
         << "r = true;"
         << endl;
    }
    else
    {
      // Initialize id image (all this is equivalent to using rsts).
      //
      os << "id_image_type& i (sts.id_image ());"
         << "init (i, id (obj));"
         << endl;

      os << "binding& idb (sts.id_image_binding ());"
         << "if (i.version != sts.id_image_version () || idb.version == 0)"
         << "{"
         << "bind (idb.bind, i);"
         << "sts.id_image_version (i.version);"
         << "idb.version++;";
      if (opt != 0)
        os << "sts.optimistic_id_image_binding ().version++;";
      os << "}";
    }

    // Resolve extra statements if we are doing direct calls.
    //
    if (!poly)
      os << "extra_statement_cache_type& esc (sts.extra_statement_cache ());"
         << endl;

    // Dispatch.
    //
    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      // Skip special sections.
      //
      if (i->special == user_section::special_version)
        continue;

      if (i->load == user_section::load_eager)
        continue;

      // Overridden sections are handled by the base.
      //
      if (poly_derived && i->base != 0)
        continue;

      data_member& m (*i->member);

      // Section access is always by reference.
      //
      member_access& ma (m.get<member_access> ("get"));
      if (!ma.synthesized)
        os << "// From " << location_string (ma.loc, true) << endl;

      os << "if (!r && &s == &" << ma.translate ("obj") << ")"
         << "{";

      if (!poly)
        os << public_name (m) << "_traits::load (esc, obj);";
      else
      {
        // If this is an empty section, then there may not be any
        // overrides.
        //
        os << "info_type::section_load sl (" <<
          "pi->find_section_load (" << i->index << "UL));";

        if (i->load_empty ())
          os << "if (sl != 0)" << endl;

        os << "sl (conn, obj, true);";
      }

      os << "r = true;"
         << "}";
    }

    if (poly)
      os << "if (top)"
         << "{";

    os << rsts << ".load_delayed (" << (versioned ? "&svm" : "0") << ");"
       << "l.unlock ();";

    if (poly)
      os << "}";

    os << "return r;"
       << "}";
  }

  // update (section) [non-thunk version]
  //
  if (uss.count (user_sections::count_new   |
                 user_sections::count_update  |
                 (poly ? user_sections::count_update_empty : 0)) != 0)
  {
    os << "bool " << traits << "::" << endl
       << "update (connection& conn, const object_type& obj, " <<
      "const section& s" << (poly ? ", const info_type* pi" : "") << ")"
       << "{"
       << "using namespace " << db << ";"
       << endl;

    if (poly)
    {
      // Resolve type information if we are doing indirect calls.
      //
      os << "if (pi == 0)" // Top-level call.
         << "{"
         << "const std::type_info& t (typeid (obj));";

      if (!abst)
        os << endl
           << "if (t == info.type)" << endl
           << "pi = &info;"
           << "else" << endl;

      os << "pi = &root_traits::map->find (t);"
         << "}";
    }

    // If our poly-base has update sections, then call the base version
    // first. Besides checking for sections, it will also initialize the
    // id image.
    //
    if (poly_derived &&
        buss->count (user_sections::count_total  |
                     user_sections::count_update |
                     user_sections::count_update_empty) != 0)
    {
      os << "if (base_traits::update (conn, obj, s, pi))" << endl
         << "return true;"
         << endl;
    }
    else
    {
      // Resolve extra statements if we are doing direct calls.
      //
      os << db << "::connection& c (static_cast<" << db <<
        "::connection&> (conn));"
         << "statements_type& sts (c.statement_cache ()." <<
        "find_object<object_type> ());";

      if (!poly)
        os << "extra_statement_cache_type& esc (sts.extra_statement_cache ());";

      os << endl;

      // Initialize id image. This is not necessarily the root of the
      // polymorphic hierarchy.
      //
      if (opt != 0)
        os << "const version_type& v (version (obj));";

      os << "id_image_type& i (sts.id_image ());";

      os << "init (i, id (obj)" << (opt != 0 ? ", &v" : "") << ");"
         << endl;

      os << "binding& idb (sts.id_image_binding ());"
         << "if (i.version != sts.id_image_version () || idb.version == 0)"
         << "{"
         << "bind (idb.bind, i);"
         << "sts.id_image_version (i.version);"
         << "idb.version++;";
      if (opt != 0)
        os << "sts.optimistic_id_image_binding ().version++;";
      os << "}";
    }

    // Dispatch.
    //
    bool e (false);
    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      // Skip special sections.
      //
      if (i->special == user_section::special_version)
        continue;

      // Overridden sections are handled by the base.
      //
      if (poly_derived && i->base != 0)
        continue;

      // Skip read-only sections. Polymorphic sections are always
      // (potentially) read-write.
      //
      if (!poly && i->update_empty ())
        continue;

      data_member& m (*i->member);

      // Section access is always by reference.
      //
      member_access& ma (m.get<member_access> ("get"));
      if (!ma.synthesized)
        os << "// From " << location_string (ma.loc, true) << endl;

      if (e)
        os << "else ";
      else
        e = true;

      os << "if (&s == &" << ma.translate ("obj") << ")";

      if (!poly)
        os << public_name (m) << "_traits::update (esc, obj);";
      else
      {
        // If this is a readonly section, then there may not be any
        // overrides.
        //
        os << "{"
           << "info_type::section_update su (" <<
          "pi->find_section_update (" << i->index << "UL));";

        if (i->update_empty ())
        {
          // For optimistic section, also check that we are not the
          // final override, since otherwise we will increment the
          // version without updating anything.
          //
          if (i->optimistic ())
            os << "if (su != 0 && su != info.sections->functions[" <<
              i->index << "UL].update)" << endl;
          else
            os << "if (su != 0)" << endl;
        }

        os << "su (conn, obj);"
           << "}";
      }
    }

    os << "else" << endl
       << "return false;"
       << endl
       << "return true;"
       << "}";
  }

  // find_ ()
  //
  if (id != 0)
  {
    os << "bool " << traits << "::" << endl
       << "find_ (statements_type& sts," << endl
       << "const id_type* id";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration& svm";

    if (poly_derived && !abst)
      os << "," << endl
         << "std::size_t d";

    os << ")"
       << "{"
       << "using namespace " << db << ";"
       << endl;

    // Initialize id image.
    //
    if (poly_derived && !abst)
      os << "if (d == depth)"
         << "{";

    os << "id_image_type& i (sts.id_image ());"
       << "init (i, *id);"
       << endl;

    os << "binding& idb (sts.id_image_binding ());"
       << "if (i.version != sts.id_image_version () || idb.version == 0)"
       << "{"
       << "bind (idb.bind, i);"
       << "sts.id_image_version (i.version);"
       << "idb.version++;";
    if (opt != 0)
      os << "sts.optimistic_id_image_binding ().version++;";
    os << "}";

    if (poly_derived && !abst)
      os << "}";

    // Rebind data image.
    //
    os << "image_type& im (sts.image ());"
       << "binding& imb (sts.select_image_binding (" <<
      (poly_derived ? (abst ? "depth" : "d") : "") << "));"
       << endl;

    if (poly_derived)
    {
      os << "if (imb.version == 0 ||" << endl
         << "check_version (sts.select_image_versions (), im))"
         << "{"
         << "bind (imb.bind, 0, 0, im, statement_select" <<
        (versioned ? ", svm" : "") << ");"
         << "update_version (sts.select_image_versions ()," << endl
         << "im," << endl
         << "sts.select_image_bindings ());"
         << "}";
    }
    else
    {
      os << "if (im.version != sts.select_image_version () ||" << endl
         << "imb.version == 0)"
         << "{"
         << "bind (imb.bind, im, statement_select" <<
        (versioned ? ", svm" : "") << ");"
         << "sts.select_image_version (im.version);"
         << "imb.version++;"
         << "}";
    }

    os << "select_statement& st (sts.find_statement (" <<
      (poly_derived ? (abst ? "depth" : "d") : "") << "));"
       << endl;

    // The dynamic loader SELECT statement can be dynamically empty.
    //
    if (versioned && poly_derived && !abst)
      os << "if (st.empty ())" << endl
         << "return true;"
         << endl;

    os << "st.execute ();"
       << "auto_result ar (st);"
       << "select_statement::result r (st.fetch ());"
       << endl;

    if (grow)
    {
      os << "if (r == select_statement::truncated)"
         << "{"
         << "if (grow (im, sts.select_image_truncated ()" <<
        (versioned ? ", svm" : "") <<
        (poly_derived ? (abst ? ", depth" : ", d") : "") << "))" << endl
         << "im.version++;"
         << endl;

      if (poly_derived)
      {
        os << "if (check_version (sts.select_image_versions (), im))"
           << "{"
           << "bind (imb.bind, 0, 0, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "update_version (sts.select_image_versions ()," << endl
           << "im," << endl
           << "sts.select_image_bindings ());"
           << "st.refetch ();"
           << "}";
      }
      else
      {
        os << "if (im.version != sts.select_image_version ())"
           << "{"
           << "bind (imb.bind, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "sts.select_image_version (im.version);"
           << "imb.version++;"
           << "st.refetch ();"
           << "}";
      }

      os << "}";
    }

    // If we are delaying, only free the result if it is empty.
    //
    if (delay_freeing_statement_result)
      os << "if (r != select_statement::no_data)"
         << "{"
         << "ar.release ();"
         << "return true;"
         << "}"
         << "else" << endl
         << "return false;";
    else
      os << "return r != select_statement::no_data;";

    os << "}";
  }

  // load_()
  //
  // Load containers, reset/reload sections.
  //
  size_t load_containers (
    has_a (c, test_container | include_eager_load, &main_section));

  if (poly_derived ||
      load_containers ||
      uss.count (user_sections::count_new  |
                 user_sections::count_load |
                 (poly ? user_sections::count_load_empty : 0)) != 0)
  {
    bool load_versioned_containers (
      load_containers >
      has_a (c,
             test_container | include_eager_load |
             exclude_deleted | exclude_added | exclude_versioned,
             &main_section));

    os << "void " << traits << "::" << endl
       << "load_ (statements_type& sts," << endl
       << "object_type& obj," << endl
       << "bool reload";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration& svm";

    if (poly_derived)
      os << "," << endl
         << "std::size_t d";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (reload);";
    if (versioned)
      os << "ODB_POTENTIALLY_UNUSED (svm);";
    os << endl;

    if (poly_derived)
      os << "if (--d != 0)" << endl
         << "base_traits::load_ (sts.base_statements (), obj, reload" <<
        (context::versioned (*poly_base) ? ", svm" : "") <<
        (poly_base != poly_root ? ", d" : "") << ");"
         << endl;

    if (load_containers ||
        (!poly && uss.count (user_sections::count_new |
                             user_sections::count_load) != 0))
      os << "extra_statement_cache_type& esc (sts.extra_statement_cache ());"
         << endl;

    if (!versioned && (
          load_versioned_containers ||
          uss.count (user_sections::count_new |
                     user_sections::count_all |
                     user_sections::count_versioned_only) != 0))
    {
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));"
         << endl;
    }

    if (load_containers)
    {
      instance<container_calls> t (container_calls::load_call, &main_section);
      t->traverse (c);
    }

    for (user_sections::iterator i (uss.begin ()); i != uss.end (); ++i)
    {
      // Skip special sections.
      //
      if (i->special == user_section::special_version)
        continue;

      // Skip overridden sections; they are handled by the base.
      //
      if (i->base != 0 && poly_derived)
        continue;

      data_member& m (*i->member);

      // If the section is soft- added or deleted, check the version.
      //
      unsigned long long av (added (m));
      unsigned long long dv (deleted (m));
      if (av != 0 || dv != 0)
      {
        os << "if (";

        if (av != 0)
          os << "svm >= schema_version_migration (" << av << "ULL, true)";

        if (av != 0 && dv != 0)
          os << " &&" << endl;

        if (dv != 0)
          os << "svm <= schema_version_migration (" << dv << "ULL, true)";

        os << ")"
           << "{";
      }

      // Section access is always by reference.
      //
      member_access& ma (m.get<member_access> ("get"));
      if (!ma.synthesized)
        os << "// From " << location_string (ma.loc, true) << endl;

      if (i->load == user_section::load_eager)
      {
        // Mark an eager section as loaded.
        //
        os << ma.translate ("obj") << ".reset (true, false);"
           << endl;
        continue;
      }

      os << "if (reload)"
         << "{"
        // Reload sections that have been loaded, clear change state.
        //
         << "if (" << ma.translate ("obj") << ".loaded ())"
         << "{";

      if (!poly)
        os << public_name (m) << "_traits::load (esc, obj);";
      else
      {
        os << "info_type::section_load sl (" << endl
           << "root_traits::map->find (typeid (obj)).find_section_load (" <<
          i->index << "UL));";

        if (i->load_empty ())
          os << "if (sl != 0)" << endl;

        os << "sl (sts.connection (), obj, true);";
      }

      os << ma.translate ("obj") << ".reset (true, false);"
         << "}"
         << "}"
         << "else" << endl
        // Reset to unloaded, unchanged state.
         << ma.translate ("obj") << ".reset ();";

      if (av != 0 || dv != 0)
        os << "}";
      else
        os << endl;
    }

    os << "}";
  }

  // load_()
  //
  // Load the dynamic part of the object. We don't need it if we are
  // poly-abstract.
  //
  if (poly_derived && !abst)
  {
    os << "void " << traits << "::" << endl
       << "load_ (database& db, root_type& r, std::size_t d)"
       << "{"
       << "using namespace " << db << ";"
       << endl
       << "object_type& obj (static_cast<object_type&> (r));"
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());"
       << endl
       << "d = depth - d;" // Convert to distance from derived.
       << endl;

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));"
         << endl;

    // Avoid trying to execute an empty SELECT statement.
    //
    if (empty_depth != 0)
      os << "if (d > " << (poly_depth - empty_depth) << "UL)"
         << "{";

    os << "if (!find_ (sts, 0" << (versioned ? ", svm" : "") << ", d))" << endl
       << "throw object_not_persistent ();" // Database inconsistency.
       << endl;

    os << "select_statement& st (sts.find_statement (d));"
       << "ODB_POTENTIALLY_UNUSED (st);"
       << endl;

    // The resulting SELECT statement may end up being dynamically empty.
    //
    if (versioned)
      os << "if (!st.empty ())"
         << "{";

    if (delay_freeing_statement_result)
      os << "auto_result ar (st);";

    os << "init (obj, sts.image (), &db" << (versioned ? ", svm" : "") <<
      ", d);";

    init_value_extra ();

    if (delay_freeing_statement_result)
      os << "ar.free ();";

    if (versioned)
      os << "}";

    if (empty_depth != 0)
      os << "}";

    os << "load_ (sts, obj, false, " << (versioned ? "svm, " : "") << "d);"
       << "}";
  }

  // discriminator_ ()
  //
  if (poly && !poly_derived)
  {
    os << "void " << traits << "::" << endl
       << "discriminator_ (statements_type& sts," << endl
       << "const id_type& id," << endl
       << "discriminator_type* pd";

    if (opt != 0)
      os << "," << endl
         << "version_type* pv";

    os << ")"
       << "{"
       << "using namespace " << db << ";"
       << endl;

    // Initialize id image.
    //
    os << "id_image_type& idi (sts.discriminator_id_image ());"
       << "init (idi, id);"
       << endl;

    os << "binding& idb (sts.discriminator_id_image_binding ());"
       << "if (idi.version != sts.discriminator_id_image_version () ||" << endl
       << "idb.version == 0)"
       << "{"
       << "bind (idb.bind, idi" << (opt != 0 ? ", false" : "") << ");"
       << "sts.discriminator_id_image_version (idi.version);"
       << "idb.version++;"
       << "}";

    // Rebind data image.
    //
    os << "discriminator_image_type& i (sts.discriminator_image ());"
       << "binding& imb (sts.discriminator_image_binding ());"
       << endl
       << "if (i.version != sts.discriminator_image_version () ||" << endl
       << "imb.version == 0)"
       << "{"
      // Generate bind code inline. For now discriminator is simple
      // value so we don't need statement kind (sk).
      //
       << bind_vector << " b (imb.bind);"
       << "std::size_t n (0);"
       << "{";
    bind_discriminator_member_->traverse (*discriminator);
    os << "}";

    if (opt != 0)
    {
      os << "n++;" // For now discriminator is a simple value.
         << "{";
      bind_version_member_->traverse (*opt);
      os << "}";
    }

    os << "sts.discriminator_image_version (i.version);"
       << "imb.version++;"
       << "}";

    os << "{"
       << "select_statement& st (sts.find_discriminator_statement ());"
       << "st.execute ();"
       << "auto_result ar (st);"
       << "select_statement::result r (st.fetch ());"
       << endl
       << "if (r == select_statement::no_data)"
       << "{";

    if (opt != 0)
      os << "if (pv != 0)" << endl
         << "throw object_changed ();"
         << "else" << endl;

    os << "throw object_not_persistent ();"
       << "}";

    if (generate_grow &&
        (context::grow (*discriminator) ||
         (opt != 0 && context::grow (*opt))))
    {
      os << "else if (r == select_statement::truncated)"
         << "{";

      // Generate grow code inline.
      //
      os << "bool grew (false);"
         << truncated_vector << " t (sts.discriminator_image_truncated ());"
         << endl;

      index_ = 0;
      grow_discriminator_member_->traverse (*discriminator);

      if (opt != 0)
        grow_version_member_->traverse (*opt);

      os << "if (grew)" << endl
         << "i.version++;"
         << endl;

      os << "if (i.version != sts.discriminator_image_version ())"
         << "{"
        // Generate bind code inline. The same code as above.
        //
         << bind_vector << " b (imb.bind);"
         << "std::size_t n (0);"
         << "{";
      bind_discriminator_member_->traverse (*discriminator);
      os << "}";

      if (opt != 0)
      {
        os << "n++;" // For now discriminator is a simple value.
           << "{";
        bind_version_member_->traverse (*opt);
        os << "}";
      }

      os << "sts.discriminator_image_version (i.version);"
         << "imb.version++;"
         << "st.refetch ();"
         << "}"
         << "}";
    }

    // Discriminator cannot be long data (no streaming).
    //
    os << "}";

    // Initialize value inline instead of generating a separate
    // init() function. For now discriminator is simple value so
    // we don't need the database (db).
    //
    os << "if (pd != 0)"
       << "{"
       << "discriminator_type& d (*pd);";
    init_named_discriminator_value_member_->traverse (*discriminator);
    os << "}";

    if (opt != 0)
    {
      os << "if (pv != 0)"
         << "{"
         << "version_type& v (*pv);";
      init_named_version_value_member_->traverse (*opt);
      os << "}";
    }

    os << "}";
  }

  if (options.generate_query ())
  {
    char const* result_type;
    if (poly)
      result_type = "polymorphic_object_result_impl<object_type>";
    else if (id != 0)
      result_type = "object_result_impl<object_type>";
    else
      result_type = "no_id_object_result_impl<object_type>";

    string sep (versioned || query_optimize ? "\n" : " ");

    // Unprepared.
    //
    if (!options.omit_unprepared ())
    {
      // query ()
      //
      os << "result< " << traits << "::object_type >" << endl
         << traits << "::" << endl
         << "query (database& db, const query_base_type& q)"
         << "{"
         << "using namespace " << db << ";"
         << "using odb::details::shared;"
         << "using odb::details::shared_ptr;"
         << endl;

      os << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection (db));"
         << endl
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (versioned)
        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));";

      os << endl;

      // Rebind the image if necessary.
      //
      os << "image_type& im (sts.image ());"
         << "binding& imb (sts.select_image_binding (" <<
        (poly_derived ? "depth" : "") << "));"
         << endl;

      if (poly_derived)
      {
        os << "if (imb.version == 0 ||" << endl
           << "check_version (sts.select_image_versions (), im))"
           << "{"
           << "bind (imb.bind, 0, 0, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "update_version (sts.select_image_versions ()," << endl
           << "im," << endl
           << "sts.select_image_bindings ());"
           << "}";
      }
      else
      {
        os << "if (im.version != sts.select_image_version () ||" << endl
           << "imb.version == 0)"
           << "{"
           << "bind (imb.bind, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "sts.select_image_version (im.version);"
           << "imb.version++;"
           << "}";
      }

      os << "std::string text (query_statement);"
         << "if (!q.empty ())"
         << "{"
         << "text += " << strlit (sep) << ";"
         << "text += q.clause ();"
         << "}";

      os << "q.init_parameters ();"
         << "shared_ptr<select_statement> st (" << endl
         << "new (shared) select_statement (" << endl;
      object_query_statement_ctor_args (
        c, "q", versioned || query_optimize, false);
      os << "));" << endl
         << "st->execute ();";

      post_query_ (c, true);

      os << endl
         << "shared_ptr< odb::" << result_type << " > r (" << endl
         << "new (shared) " << db << "::" << result_type << " (" << endl
         << "q, st, sts, " << (versioned ? "&svm" : "0") << "));"
         << endl
         << "return result<object_type> (r);"
         << "}";

      // query(odb::query_base)
      //
      if (multi_dynamic)
        os << "result< " << traits << "::object_type >" << endl
           << traits << "::" << endl
           << "query (database& db, const odb::query_base& q)"
           << "{"
           << "return query (db, query_base_type (q));"
           << "}";
    }

    // erase_query
    //
    os << "unsigned long long " << traits << "::" << endl
       << "erase_query (database& db, const query_base_type& q)"
       << "{"
       << "using namespace " << db << ";"
       << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << endl
       << "std::string text (erase_query_statement);"
       << "if (!q.empty ())"
       << "{"
       << "text += ' ';"
       << "text += q.clause ();"
       << "}"
       << "q.init_parameters ();"
       << "delete_statement st (" << endl;
    object_erase_query_statement_ctor_args (c);
    os << ");"
       << endl
       << "return st.execute ();"
       << "}";

    // erase_query(odb::query_base)
    //
    if (multi_dynamic)
      os << "unsigned long long " << traits << "::" << endl
         << "erase_query (database& db, const odb::query_base& q)"
         << "{"
         << "return erase_query (db, query_base_type (q));"
         << "}";

    // Prepared. Very similar to unprepared but has some annoying variations
    // that make it difficult to factor out something common.
    //
    if (options.generate_prepared ())
    {
      // prepare_query
      //
      os << "odb::details::shared_ptr<prepared_query_impl>" << endl
         << traits << "::" << endl
         << "prepare_query (connection& c, const char* n, " <<
        "const query_base_type& q)"
         << "{"
         << "using namespace " << db << ";"
         << "using odb::details::shared;"
         << "using odb::details::shared_ptr;"
         << endl;

      os << db << "::connection& conn (" << endl
         << "static_cast<" << db << "::connection&> (c));"
         << endl
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (versioned)
        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));";

      os << endl;

      // Rebind the image if necessary.
      //
      os << "image_type& im (sts.image ());"
         << "binding& imb (sts.select_image_binding (" <<
        (poly_derived ? "depth" : "") << "));"
         << endl;

      if (poly_derived)
      {
        os << "if (imb.version == 0 ||" << endl
           << "check_version (sts.select_image_versions (), im))"
           << "{"
           << "bind (imb.bind, 0, 0, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "update_version (sts.select_image_versions ()," << endl
           << "im," << endl
           << "sts.select_image_bindings ());"
           << "}";
      }
      else
      {
        os << "if (im.version != sts.select_image_version () ||" << endl
           << "imb.version == 0)"
           << "{"
           << "bind (imb.bind, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "sts.select_image_version (im.version);"
           << "imb.version++;"
           << "}";
      }

      os << "std::string text (query_statement);"
         << "if (!q.empty ())"
         << "{"
         << "text += " << strlit (sep) << ";"
         << "text += q.clause ();"
         << "}";

      os << "shared_ptr<" << db << "::prepared_query_impl> r (" << endl
         << "new (shared) " << db << "::prepared_query_impl (conn));"
         << "r->name = n;"
         << "r->execute = &execute_query;"
         << "r->query = q;"
         << "r->stmt.reset (" << endl
         << "new (shared) select_statement (" << endl;
      object_query_statement_ctor_args (
        c, "r->query", versioned || query_optimize, true);
      os << "));"
         << endl
         << "return r;"
         << "}";

      // prepare_query(odb::query_base)
      //
      if (multi_dynamic)
        os << "odb::details::shared_ptr<prepared_query_impl>" << endl
           << traits << "::" << endl
           << "prepare_query (connection& c, const char* n, " <<
          "const odb::query_base& q)"
           << "{"
           << "return prepare_query (c, n, query_base_type (q));"
           << "}";

      // execute_query
      //
      os << "odb::details::shared_ptr<result_impl>" << endl
         << traits << "::" << endl
         << "execute_query (prepared_query_impl& q)"
         << "{"
         << "using namespace " << db << ";"
         << "using odb::details::shared;"
         << "using odb::details::shared_ptr;"
         << endl
         << db << "::prepared_query_impl& pq (" << endl
         << "static_cast<" << db << "::prepared_query_impl&> (q));"
         << "shared_ptr<select_statement> st (" << endl
         << "odb::details::inc_ref (" << endl
         << "static_cast<select_statement*> (pq.stmt.get ())));"
         << endl;

      os << db << "::transaction& tr (" << db << "::transaction::current ());"
         << "ODB_POTENTIALLY_UNUSED (tr);"
         << endl
         << "// The connection used by the current transaction and the" << endl
         << "// one used to prepare this statement must be the same." << endl
         << "//" << endl
         << "assert (q.verify_connection (tr));"
         << endl
         << "statements_type& sts (" << endl
         << "st->connection ().statement_cache ().find_object<object_type> ());";

      if (versioned)
        os << "const schema_version_migration& svm (" <<
          "sts.version_migration (" << schema_name << "));";

      os << endl;

      // Rebind the image if necessary.
      //
      os << "image_type& im (sts.image ());"
         << "binding& imb (sts.select_image_binding (" <<
        (poly_derived ? "depth" : "") << "));"
         << endl;

      if (poly_derived)
      {
        os << "if (imb.version == 0 ||" << endl
           << "check_version (sts.select_image_versions (), im))"
           << "{"
           << "bind (imb.bind, 0, 0, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "update_version (sts.select_image_versions ()," << endl
           << "im," << endl
           << "sts.select_image_bindings ());"
           << "}";
      }
      else
      {
        os << "if (im.version != sts.select_image_version () ||" << endl
           << "imb.version == 0)"
           << "{"
           << "bind (imb.bind, im, statement_select" <<
          (versioned ? ", svm" : "") << ");"
           << "sts.select_image_version (im.version);"
           << "imb.version++;"
           << "}";
      }

      os << "pq.query.init_parameters ();"
         << "st->execute ();";
      post_query_ (c, false);

      os << endl
         << "return shared_ptr<result_impl> (" << endl
         << "new (shared) " << db << "::" << result_type << " (" << endl
         << "pq.query, st, sts, " << (versioned ? "&svm" : "0") << "));"
         << "}";
    }
  }

  // Generate function table registration for dynamic multi-database
  // support.
  //
  if (multi_dynamic)
  {
    string fn (flat_name (type));
    string dt ("access::object_traits_impl< " + type + ", id_common >");

    os << "static const" << endl
       << dt << "::" << endl
       << "function_table_type function_table_" << fn << "_ ="
       << "{";

    // persist ()
    //
    os << "&" << traits << "::persist";

    if (id != 0)
    {
      // find (id)
      //
      if (c.default_ctor ())
        os << "," << endl
           << "&" << traits << "::find";

      // find (id, obj)
      //
      os << "," << endl
         << "&" << traits << "::find";

      // reload ()
      //
      os << "," << endl
         << "&" << traits << "::reload";

      // update ()
      //
      if (!readonly || poly)
        os << "," << endl
           << "&" << traits << "::update";

      // erase ()
      //
      os << "," << endl
         << "&" << traits << "::erase";

      os << "," << endl
         << "&" << traits << "::erase";

      // Sections.
      //
      if (uss.count (user_sections::count_total |
                     user_sections::count_load  |
                     (poly ? user_sections::count_load_empty : 0)) != 0)
        os << "," << endl
           << "&" << traits << "::load";

      if (uss.count (user_sections::count_total  |
                     user_sections::count_update |
                     (poly ? user_sections::count_update_empty : 0)) != 0)
        os << "," << endl
           << "&" << traits << "::update";
    }

    if (options.generate_query ())
    {
      if (!options.omit_unprepared ())
        os << "," << endl
           << "&" << traits << "::query";

      os << "," << endl
         << "&" << traits << "::erase_query";

      if (options.generate_prepared ())
      {
        os << "," << endl
           << "&" << traits << "::prepare_query";

        os << "," << endl
           << "&" << traits << "::execute_query";
      }
    }

    os << "};";

    os << "static const object_function_table_entry< " << type << ", " <<
      "id_" << db << " >" << endl
       << "function_table_entry_" << fn << "_ (" << endl
       << "&function_table_" << fn << "_);"
       << endl;
  }
}

void relational::source::class_::
traverse_view (type& c)
{
  view_query& vq (c.get<view_query> ("query"));

  // Only process the view query if it is versioned since the query text
  // (e.g., in the native view) must be structured. We also shouldn't try
  // to optimize JOINs (since the objects are JOINed explicitly by the
  // user), unless we are adding poly-base/derived JOINs.
  //
  bool versioned (context::versioned (c));
  bool query_optimize (false);

  string const& type (class_fq_name (c));
  string traits ("access::view_traits_impl< " + type + ", id_" +
                 db.string () + " >");

  size_t columns (column_count (c).total);

  // Schema name as a string literal or empty.
  //
  string schema_name (options.schema_name ()[db]);
  if (!schema_name.empty ())
    schema_name = strlit (schema_name);

  // Generate the from-list. Also collect relationships via which
  // the objects are joined.
  //
  strings from;
  view_relationship_map rel_map;

  if (vq.kind == view_query::condition)
  {
    view_objects& objs (c.get<view_objects> ("objects"));

    for (view_objects::iterator i (objs.begin ()); i != objs.end (); ++i)
    {
      bool first (i == objs.begin ());
      string l;

      //
      // Tables.
      //

      if (i->kind == view_object::table)
      {
        if (first)
        {
          l = "FROM ";
          l += quote_id (i->tbl_name);

          if (!i->alias.empty ())
            l += (need_alias_as ? " AS " : " ") + quote_id (i->alias);

          l += from_trailer (c);

          from.push_back (l);
          continue;
        }

        l = join_syntax (*i);
        l += ' ';
        l += quote_id (i->tbl_name);

        if (!i->alias.empty ())
          l += (need_alias_as ? " AS " : " ") + quote_id (i->alias);

        if (i->join == view_object::cross) // No ON condition for CROSS JOIN.
        {
          from.push_back (l);
          continue;
        }

        semantics::scope& scope (
          dynamic_cast<semantics::scope&> (*unit.find (i->scope)));

        expression e (
          translate_expression (
            c, i->cond, scope, i->loc, "table"));

        if (e.kind != expression::literal)
        {
          error (i->loc) << "invalid join condition in db pragma " <<
            "table" << endl;
          throw operation_failed ();
        }

        l += " ON";

        // Add the pragma location for easier error tracking.
        //
        from.push_back (l);
        from.push_back ("// From " + location_string (i->loc, true));
        from.push_back (e.value);
        continue;
      }

      //
      // Objects.
      //
      semantics::class_& o (*i->obj);

      bool poly (polymorphic (o));
      size_t poly_depth (poly ? polymorphic_depth (o) : 1);

      string alias (i->alias);

      // For polymorphic objects, alias is just a prefix.
      //
      if (poly && !alias.empty ())
        alias += "_" + table_name (o).uname ();

      // First object.
      //
      if (first)
      {
        l = "FROM ";
        l += table_qname (o);

        if (!alias.empty ())
          l += (need_alias_as ? " AS " : " ") + quote_id (alias);

        l += from_trailer (c);

        from.push_back (l);

        if (poly_depth != 1)
        {
          bool t (true);             //@@ (im)perfect forwarding
          size_t d (poly_depth - 1); //@@ (im)perfect forward.
          instance<polymorphic_object_joins> j (o, t, d, i->alias);
          j->traverse (polymorphic_base (o));

          from.insert (from.end (), j->begin (), j->end ());
          query_optimize = query_optimize || !j->joins.empty ();
        }
        continue;
      }

      semantics::scope& scope (
        dynamic_cast<semantics::scope&> (*unit.find (i->scope)));

      expression e (i->join == view_object::cross
                    ? expression ("") // Dummy literal expression.
                    : translate_expression (
                        c, i->cond, scope, i->loc, "object"));

      // Literal expression.
      //
      if (e.kind == expression::literal)
      {
        l = join_syntax (*i);
        l += ' ';
        l += table_qname (o);

        if (!alias.empty ())
          l += (need_alias_as ? " AS " : " ") + quote_id (alias);

        if (i->join == view_object::cross) // No ON condition for CROSS JOIN.
        {
          from.push_back (l);
          continue;
        }

        l += " ON";

        // Add the pragma location for easier error tracking.
        //
        from.push_back (l);
        from.push_back ("// From " + location_string (i->loc, true));
        from.push_back (e.value);

        if (poly_depth != 1)
        {
          bool t (true);             //@@ (im)perfect forwarding
          size_t d (poly_depth - 1); //@@ (im)perfect forward.
          instance<polymorphic_object_joins> j (o, t, d, i->alias);
          j->traverse (polymorphic_base (o));

          from.insert (from.end (), j->begin (), j->end ());
          query_optimize = query_optimize || !j->joins.empty ();
        }
        continue;
      }

      // We have an object relationship (pointer) for which we need
      // to come up with the corresponding JOIN condition. If this
      // is a to-many relationship, then we first need to JOIN the
      // container table. This code is similar to object_joins.
      //
      // Note that this cannot be CROSS JOIN; we've handled that case
      // above.
      //
      using semantics::data_member;

      data_member& m (*e.member_path.back ());
      data_member_path* imp (inverse (m));

      // Resolve the pointed-to object to view_object and do
      // some sanity checks while at it.
      //
      semantics::class_* c (0);

      if (container (m))
        c = object_pointer (container_vt (m));
      else
        c = object_pointer (utype (m));

      view_object* vo (0);

      // Check if the pointed-to object has been previously associated
      // with this view and is unambiguous. A pointer to ourselves is
      // always assumed to point to this association.
      //
      if (&o == c)
        vo = &*i;
      else
      {
        bool ambig (false);

        for (view_objects::iterator j (objs.begin ()); j != i; ++j)
        {
          if (j->obj != c)
            continue;

          if (vo == 0)
          {
            vo = &*j;
            continue;
          }

          // If it is the first ambiguous object, issue the
          // error.
          //
          if (!ambig)
          {
            error (i->loc) << "pointed-to object '" << class_name (*c) <<
              "' is ambiguous" << endl;
            info (i->loc) << "candidates are:" << endl;
            info (vo->loc) << "  '" << vo->name () << "'" << endl;
            ambig = true;
          }

          info (j->loc) << "  '" << j->name () << "'" << endl;
        }

        if (ambig)
        {
          info (i->loc) << "use the other side of the relationship " <<
            "or full join condition clause in db pragma object to " <<
            "resolve this ambiguity" << endl;
          throw operation_failed ();
        }

        if (vo == 0)
        {
          error (i->loc) << "pointed-to object '" << class_name (*c) <<
            "' specified in the join condition has not been " <<
            "previously associated with this view" << endl;
          throw operation_failed ();
        }
      }

      // JOIN relationship points to us:
      //   vo            - us
      //   e.vo          - other side
      //   e.member_path - in other side
      //
      // JOIN relationship points to other side:
      //   vo            - other side
      //   e.vo          - us
      //   e.member_path - in us
      //
      if (imp == 0)
        rel_map.insert (make_pair (e.member_path, make_pair (e.vo, vo)));
      else
        rel_map.insert (make_pair (*imp, make_pair (vo, e.vo)));

      // Left and right-hand side table names.
      //
      qname lt;
      {
        using semantics::class_;

        class_& o (*e.vo->obj);
        string const& a (e.vo->alias);

        if (class_* root = polymorphic (o))
        {
          // If the object is polymorphic, then figure out which of the
          // bases this member comes from and use the corresponding
          // table.
          //
          class_* c (
            &static_cast<class_&> (
              e.member_path.front ()->scope ()));

          // If this member's class is not polymorphic (root uses reuse
          // inheritance), then use the root table.
          //
          if (!polymorphic (*c))
            c = root;

          qname const& t (table_name (*c));

          if (a.empty ())
            lt = t;
          else
            lt = qname (a + "_" + t.uname ());
        }
        else
          lt = a.empty () ? table_name (o) : qname (a);
      }

      qname rt;
      {
        qname t (table_name (*vo->obj));
        string const& a (vo->alias);
        rt = a.empty ()
          ? t
          : qname (polymorphic (*vo->obj) ? a + "_" + t.uname () : a);
      }

      // First join the container table if necessary.
      //
      semantics::type* cont (container (imp != 0 ? *imp->back () : m));

      string ct; // Container table.
      if (cont != 0)
      {
        l = join_syntax (*i);
        l += ' ';

        // The same relationship can be used by multiple associated
        // objects. So if the object that contains this container has
        // an alias, then also construct one for the table that we
        // are joining.
        //
        {
          using semantics::class_;

          qname t;

          // In a polymorphic hierarchy the member can be in a base (see
          // above).
          //
          if (class_* root = polymorphic (imp != 0 ? *vo->obj : *e.vo->obj))
          {
            class_* c;
            if (imp == 0)
            {
              c = &static_cast<class_&> (e.member_path.front ()->scope ());

              if (!polymorphic (*c))
                c = root;

              t = table_name (*c, e.member_path);
            }
            else
            {
              c = &static_cast<class_&> (imp->front ()->scope ());

              if (!polymorphic (*c))
                c = root;

              t = table_name (*c, *imp);
            }
          }
          else
            t = imp != 0
              ? table_name (*vo->obj, *imp)
              : table_name (*e.vo->obj, e.member_path);

          // The tricky part is to figure out which view_object, vo
          // or e.vo we should use. We want to use the alias from the
          // object that actually contains this container. The following
          // might not make intuitive sense, but it has been verified
          // with the truth table.
          //
          string const& a (imp != 0 ? vo->alias : e.vo->alias);

          if (a.empty ())
          {
            ct = quote_id (t);
            l += ct;
          }
          else
          {
            ct = quote_id (a + '_' + t.uname ());
            l += quote_id (t);
            l += (need_alias_as ? " AS " : " ") + ct;
          }
        }

        l += " ON";
        from.push_back (l);

        // If we are the pointed-to object, then we have to turn
        // things around. This is necessary to have the proper
        // JOIN order. There seems to be a pattern there but it
        // is not yet intuitively clear what it means.
        //
        instance<object_columns_list> c_cols; // Container columns.
        instance<object_columns_list> o_cols; // Object columns.

        qname* ot; // Object table (either lt or rt).

        if (imp != 0)
        {
          semantics::data_member& imb (*imp->back ());

          if (&o == c)
          {
            // container.value = pointer.id
            //
            data_member_path& id (*id_member (*e.vo->obj));

            c_cols->traverse (imb, utype (id), "value", "value");
            o_cols->traverse (id);
            ot = &lt;
          }
          else
          {
            // container.id = pointed-to.id
            //
            data_member_path& id (*id_member (*vo->obj));

            c_cols->traverse (imb, utype (id), "id", "object_id", vo->obj);
            o_cols->traverse (id);
            ot = &rt;
          }
        }
        else
        {
          if (&o == c)
          {
            // container.id = pointer.id
            //
            data_member_path& id (*id_member (*e.vo->obj));

            c_cols->traverse (
              m, utype (id), "id", "object_id", e.vo->obj);
            o_cols->traverse (id);
            ot = &lt;
          }
          else
          {
            // container.value = pointed-to.id
            //
            data_member_path& id (*id_member (*vo->obj));

            c_cols->traverse (m, utype (id), "value", "value");
            o_cols->traverse (id);
            ot = &rt;
          }
        }

        for (object_columns_list::iterator b (c_cols->begin ()), i (b),
               j (o_cols->begin ()); i != c_cols->end (); ++i, ++j)
        {
          l.clear ();

          if (i != b)
            l += "AND ";

          l += ct;
          l += '.';
          l += quote_id (i->name);
          l += '=';
          l += quote_id (*ot);
          l += '.';
          l += quote_id (j->name);

          from.push_back (strlit (l));
        }
      }

      // If we have already joined the container with the desired
      // join type, then use LEFT JOIN to join the object to the
      // container. This is the right thing to do since an entry
      // in the container can only point (either via id or value)
      // to a single object.
      //
      l = (cont == 0 ? join_syntax (*i) : "LEFT JOIN");
      l += ' ';

      l += table_qname (o);

      if (!alias.empty ())
        l += (need_alias_as ? " AS " : " ") + quote_id (alias);

      l += " ON";
      from.push_back (l);

      if (cont != 0)
      {
        instance<object_columns_list> c_cols; // Container columns.
        instance<object_columns_list> o_cols; // Object columns.

        qname* ot; // Object table (either lt or rt).

        if (imp != 0)
        {
          semantics::data_member& imb (*imp->back ());

          if (&o == c)
          {
            // container.id = pointed-to.id
            //
            data_member_path& id (*id_member (*vo->obj));

            c_cols->traverse (imb, utype (id), "id", "object_id", vo->obj);
            o_cols->traverse (id);
            ot = &rt;
          }
          else
          {
            // container.value = pointer.id
            //
            data_member_path& id (*id_member (*e.vo->obj));

            c_cols->traverse (imb, utype (id), "value", "value");
            o_cols->traverse (id);
            ot = &lt;
          }
        }
        else
        {
          if (&o == c)
          {
            // container.value = pointed-to.id
            //
            data_member_path& id (*id_member (*vo->obj));

            c_cols->traverse (m, utype (id), "value", "value");
            o_cols->traverse (id);
            ot = &rt;
          }
          else
          {
            // container.id = pointer.id
            //
            data_member_path& id (*id_member (*e.vo->obj));

            c_cols->traverse (m, utype (id), "id", "object_id", e.vo->obj);
            o_cols->traverse (id);
            ot = &lt;
          }
        }

        for (object_columns_list::iterator b (c_cols->begin ()), i (b),
               j (o_cols->begin ()); i != c_cols->end (); ++i, ++j)
        {
          l.clear ();

          if (i != b)
            l += "AND ";

          l += ct;
          l += '.';
          l += quote_id (i->name);
          l += '=';
          l += quote_id (*ot);
          l += '.';
          l += quote_id (j->name);

          from.push_back (strlit (l));
        }
      }
      else
      {
        instance<object_columns_list> l_cols;
        instance<object_columns_list> r_cols;

        if (imp != 0)
        {
          // our.id = pointed-to.pointer
          //
          l_cols->traverse (*id_member (*e.vo->obj));
          r_cols->traverse (*imp->back (), column_prefix (*imp));
        }
        else
        {
          // our.pointer = pointed-to.id
          //
          l_cols->traverse (*e.member_path.back (),
                            column_prefix (e.member_path));
          r_cols->traverse (*id_member (*vo->obj));
        }

        for (object_columns_list::iterator b (l_cols->begin ()), i (b),
               j (r_cols->begin ()); i != l_cols->end (); ++i, ++j)
        {
          l.clear ();

          if (i != b)
            l += "AND ";

          l += quote_id (lt);
          l += '.';
          l += quote_id (i->name);
          l += '=';
          l += quote_id (rt);
          l += '.';
          l += quote_id (j->name);

          from.push_back (strlit (l));
        }
      }

      if (poly_depth != 1)
      {
        bool t (true);             //@@ (im)perfect forwarding
        size_t d (poly_depth - 1); //@@ (im)perfect forward.
        instance<polymorphic_object_joins> j (o, t, d, i->alias);
        j->traverse (polymorphic_base (o));

        from.insert (from.end (), j->begin (), j->end ());
        query_optimize = query_optimize || !j->joins.empty ();
      }
    } // End JOIN-generating for-loop.
  }

  // Check that pointed-to objects inside objects that we are loading
  // have session support enabled (required to have a shared copy).
  // Also see if we need to throw if there is no session.
  //
  bool need_session (false);
  if (vq.kind == view_query::condition)
  {
    view_objects& objs (c.get<view_objects> ("objects"));
    for (view_objects::iterator i (objs.begin ()); i != objs.end (); ++i)
    {
      if (i->kind != view_object::object || i->ptr == 0)
        continue; // Not an object or not loaded.

      instance<view_object_check> t (*i, rel_map);
      t->traverse (*i->obj);
      need_session = need_session || t->session_;
    }
  }

  os << "// " << class_name (c) << endl
     << "//" << endl
     << endl;

  view_extra (c);

  // query_columns
  //
  if (c.get<size_t> ("object-count") != 0)
    view_query_columns_type_->traverse (c);

  //
  // Functions.
  //

  // grow ()
  //
  if (generate_grow && columns != 0)
  {
    os << "bool " << traits << "::" << endl
       << "grow (image_type& i," << endl
       << truncated_vector << " t";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration& svm";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (i);"
       << "ODB_POTENTIALLY_UNUSED (t);";

    if (versioned)
      os << "ODB_POTENTIALLY_UNUSED (svm);";

    os << endl
       << "bool grew (false);"
       << endl;

    index_ = 0;
    names (c, grow_member_names_);

    os << "return grew;"
       << "}";
  }

  // bind (image_type)
  //
  if (columns != 0)
  {
    os << "void " << traits << "::" << endl
       << "bind (" << bind_vector << " b," << endl
       << "image_type& i";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration& svm";

    os << ")"
       << "{";

    if (versioned)
      os << "ODB_POTENTIALLY_UNUSED (svm);"
         << endl;

    os << "using namespace " << db << ";"
       << endl
       << db << "::statement_kind sk (statement_select);"
       << "ODB_POTENTIALLY_UNUSED (sk);"
       << endl
       << "std::size_t n (0);"
       << endl;

    names (c, bind_member_names_);

    os << "}";
  }

  // init (view, image)
  //
  if (columns != 0)
  {
    os << "void " << traits << "::" << endl
       << "init (view_type& o," << endl
       << "const image_type& i," << endl
       << "database* db";

    if (versioned)
      os << "," << endl
         << "const schema_version_migration& svm";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (o);"
       << "ODB_POTENTIALLY_UNUSED (i);"
       << "ODB_POTENTIALLY_UNUSED (db);";

    if (versioned)
      os << "ODB_POTENTIALLY_UNUSED (svm);";

    os << endl;

    if (need_session)
      os << "if (!" << options.session_type () << "::_has_cache ())" << endl
         << "throw session_required ();"
         << endl;

    // Note: db must be not NULL in order to load pointers.
    //
    if (has_a (c, test_pointer))
      os << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection (*db));"
         << endl;

    names (c, init_view_pointer_member_pre_names_);
    names (c, init_value_member_names_);
    names (c, init_view_pointer_member_post_names_);

    os << "}";
  }

  // query_statement()
  //
  if (vq.kind != view_query::runtime)
  {
    os << traits << "::query_base_type" << endl
       << traits << "::" << endl
       << "query_statement (const query_base_type& q)"
       << "{";

    if (vq.kind == view_query::complete_select ||
        vq.kind == view_query::complete_execute)
    {
      os << "query_base_type r (" << endl;

      bool ph (false);
      bool pred (vq.kind == view_query::complete_select);

      if (!vq.literal.empty ())
      {
        // See if we have the '(?)' placeholder.
        //
        // @@ Ideally we would need to make sure we don't match
        // this inside strings and quoted identifier. So the
        // proper way to handle this would be to tokenize the
        // statement using sql_lexer, once it is complete enough.
        //
        string::size_type p (vq.literal.find ("(?)"));

        if (p != string::npos)
        {
          ph = true;
          // For the SELECT query we keep the parenthesis in (?) and
          // also handle the case where the query expression is empty.
          //
          if (pred)
            os << strlit (string (vq.literal, 0, p + 1)) << " +" << endl
               << "(q.empty () ? query_base_type::true_expr : q) +" << endl
               << strlit (string (vq.literal, p + 2));
          else
          {
            os << strlit (string (vq.literal, 0, p)) << " + q";

            p += 3;
            if (p != vq.literal.size ())
              os << " + " << strlit (string (vq.literal, p));
          }
        }
        else
          os << strlit (vq.literal);
      }
      else
      {
        semantics::scope& scope (
          dynamic_cast<semantics::scope&> (*unit.find (vq.scope)));

        // Output the pragma location for easier error tracking.
        //
        os << "// From " << location_string (vq.loc, true) << endl
           << translate_expression (
             c, vq.expr, scope, vq.loc, "query", &ph, pred).value;
      }

      os << ");";

      // If there was no placeholder, add the query condition
      // at the end.
      //
      if (!ph)
        os << endl
           << "if (!q.empty ())"
           << "{"
           << "r += " << strlit (versioned ? "\n" : " ") << ";"
           << "r += q.clause_prefix ();"
           << "r += q;"
           << "}";
    }
    else // vq.kind == view_query::condition
    {
      // Use the from-list generated above.
      //
      statement_columns sc;
      {
        instance<view_columns> t (sc, from, rel_map);
        t->traverse (c);
        process_statement_columns (
          sc, statement_select, versioned || query_optimize);
      }

      string sep (versioned || query_optimize ? "\n" : " ");

      os << "query_base_type r (" << endl
         << strlit ((vq.distinct ? "SELECT DISTINCT" : "SELECT") + sep);

      for (statement_columns::const_iterator i (sc.begin ()),
             e (sc.end ()); i != e;)
      {
        string const& c (i->column);
        os << endl
           << strlit (c + (++i != e ? "," : "") + sep);
      }

      os << ");"
         << endl;

      // It is much easier to add the separator at the beginning of the
      // next line since the JOIN condition may not be a string literal.
      //
      for (strings::const_iterator i (from.begin ()); i != from.end (); ++i)
      {
        if (i->compare (0, 5, "FROM ") == 0)
          os << "r += " << strlit (*i) << ";";
        else if (i->compare (0, 3, "// ") == 0)
          os << *i << endl;
        else
        {
          // See if this is a JOIN. The exact spelling is database-dependent,
          // but we know there is the JOIN word in there somewhere and before
          // it we should only have keywords and spaces.
          //
          size_t p (i->find ("JOIN "));
          if (p != string::npos)
          {
            // Make sure before it we only have A-Z and spaces.
            //
            for (char c; p != 0; --p)
            {
              c = (*i)[p - 1];
              if ((c < 'A' || c > 'Z') && c != ' ')
                break;
            }

            if (p == 0)
              os << endl
                 << "r += " << strlit (sep + *i) << ";";
          }

          // Something else, assume already a string literal.
          //
          if (p != 0)
            os << "r += " << *i << ";";
        }
      }

      // Generate the query condition.
      //
      if (!vq.literal.empty () || !vq.expr.empty ())
      {
        os << endl
           << "query_base_type c (" << endl;

        bool ph (false);

        if (!vq.literal.empty ())
        {
          // See if we have the '(?)' placeholder.
          //
          // @@ Ideally we would need to make sure we don't match
          // this inside strings and quoted identifier. So the
          // proper way to handle this would be to tokenize the
          // statement using sql_lexer, once it is complete enough.
          //
          string::size_type p (vq.literal.find ("(?)"));

          if (p != string::npos)
          {
            ph = true;
            os << strlit (string (vq.literal, 0, p + 1))<< " +" << endl
               << "(q.empty () ? query_base_type::true_expr : q) +" << endl
               << strlit (string (vq.literal, p + 2));
          }
          else
            os << strlit (vq.literal);

          os << ");";
        }
        else
        {
          semantics::scope& scope (
            dynamic_cast<semantics::scope&> (*unit.find (vq.scope)));

          // Output the pragma location for easier error tracking.
          //
          os << "// From " << location_string (vq.loc, true) << endl
             << translate_expression (
               c, vq.expr, scope, vq.loc, "query", &ph).value;

          os << ");";

          // Optimize the query if it had a placeholder. This gets
          // rid of useless clauses like WHERE TRUE.
          //
          if (ph)
            os << endl
               << "c.optimize ();";
        }

        if (!ph)
          os << endl
             << "c += q;";

        os << endl
           << "if (!c.empty ())"
           << "{"
           << "r += " << strlit (sep) << ";"
           << "r += c.clause_prefix ();"
           << "r += c;"
           << "}";

        string st (select_trailer (c));
        if (!st.empty ())
        {
          os << "r += " << strlit (sep) << ";"
             << "r += " << strlit (st) << ";";
        }
      }
      else
      {
        os << endl
           << "if (!q.empty ())"
           << "{"
           << "r += " << strlit (sep) << ";"
           << "r += q.clause_prefix ();"
           << "r += q;"
           << "}";
      }
    }

    os << "return r;"
       << "}";
  }

  // Unprepared.
  //
  if (!options.omit_unprepared ())
  {
    os << "result< " << traits << "::view_type >" << endl
       << traits << "::" << endl
       << "query (database& db, const query_base_type& q)"
       << "{"
       << "using namespace " << db << ";"
       << "using odb::details::shared;"
       << "using odb::details::shared_ptr;"
       << endl;

    os << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection (db));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_view<view_type> ());";

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    os << endl
       << "image_type& im (sts.image ());"
       << "binding& imb (sts.image_binding ());"
       << endl
       << "if (im.version != sts.image_version () || imb.version == 0)"
       << "{"
       << "bind (imb.bind, im" << (versioned ? ", svm" : "") << ");"
       << "sts.image_version (im.version);"
       << "imb.version++;"
       << "}";

    if (vq.kind == view_query::runtime)
      os << "const query_base_type& qs (q);";
    else
      os << "const query_base_type& qs (query_statement (q));";

    os << "qs.init_parameters ();"
       << "shared_ptr<select_statement> st (" << endl
       << "new (shared) select_statement (" << endl;
    view_query_statement_ctor_args (
      c, "qs", versioned || query_optimize, false);
    os << "));" << endl
       << "st->execute ();";

    post_query_ (c, true);

    os << endl
       << "shared_ptr< odb::view_result_impl<view_type> > r (" << endl
       << "new (shared) " << db << "::view_result_impl<view_type> (" << endl
       << "qs, st, sts, " << (versioned ? "&svm" : "0") << "));"
       << endl
       << "return result<view_type> (r);"
       << "}";

    // query(odb::query_base)
    //
    if (multi_dynamic)
      os << "result< " << traits << "::view_type >" << endl
         << traits << "::" << endl
         << "query (database& db, const odb::query_base& q)"
         << "{"
         << "return query (db, query_base_type (q));"
         << "}";
  }

  // Prepared. Very similar to unprepared but has some annoying variations
  // that make it difficult to factor out something common.
  //
  if (options.generate_prepared ())
  {
    // prepare_query
    //
    os << "odb::details::shared_ptr<prepared_query_impl>" << endl
       << traits << "::" << endl
       << "prepare_query (connection& c, const char* n, " <<
      "const query_base_type& q)"
       << "{"
       << "using namespace " << db << ";"
       << "using odb::details::shared;"
       << "using odb::details::shared_ptr;"
       << endl;

    os << db << "::connection& conn (" << endl
       << "static_cast<" << db << "::connection&> (c));"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_view<view_type> ());";

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    os << endl;

    // Rebind the image if necessary.
    //
    os << "image_type& im (sts.image ());"
       << "binding& imb (sts.image_binding ());"
       << endl
       << "if (im.version != sts.image_version () || imb.version == 0)"
       << "{"
       << "bind (imb.bind, im" << (versioned ? ", svm" : "") << ");"
       << "sts.image_version (im.version);"
       << "imb.version++;"
       << "}";

    os << "shared_ptr<" << db << "::prepared_query_impl> r (" << endl
       << "new (shared) " << db << "::prepared_query_impl (conn));"
       << "r->name = n;"
       << "r->execute = &execute_query;";

    if (vq.kind == view_query::runtime)
      os << "r->query = q;";
    else
      os << "r->query = query_statement (q);";

    os << "r->stmt.reset (" << endl
       << "new (shared) select_statement (" << endl;
    view_query_statement_ctor_args (
      c, "r->query", versioned || query_optimize, true);
    os << "));"
       << endl
       << "return r;"
       << "}";

    // prepare_query(odb::query_base)
    //
    if (multi_dynamic)
      os << "odb::details::shared_ptr<prepared_query_impl>" << endl
         << traits << "::" << endl
         << "prepare_query (connection& c, const char* n, " <<
        "const odb::query_base& q)"
         << "{"
         << "return prepare_query (c, n, query_base_type (q));"
         << "}";

    // execute_query
    //
    os << "odb::details::shared_ptr<result_impl>" << endl
       << traits << "::" << endl
       << "execute_query (prepared_query_impl& q)"
       << "{"
       << "using namespace " << db << ";"
       << "using odb::details::shared;"
       << "using odb::details::shared_ptr;"
       << endl
       << db << "::prepared_query_impl& pq (" << endl
       << "static_cast<" << db << "::prepared_query_impl&> (q));"
       << "shared_ptr<select_statement> st (" << endl
       << "odb::details::inc_ref (" << endl
       << "static_cast<select_statement*> (pq.stmt.get ())));"
       << endl;

    os << db << "::transaction& tr (" << db << "::transaction::current ());"
       << "ODB_POTENTIALLY_UNUSED (tr);"
       << endl
       << "// The connection used by the current transaction and the" << endl
       << "// one used to prepare this statement must be the same." << endl
       << "//" << endl
       << "assert (q.verify_connection (tr));"
       << endl
       << "statements_type& sts (" << endl
       << "st->connection ().statement_cache ().find_view<view_type> ());";

    if (versioned)
      os << "const schema_version_migration& svm (" <<
        "sts.version_migration (" << schema_name << "));";

    os << endl;

    // Rebind the image if necessary.
    //
    os << "image_type& im (sts.image ());"
       << "binding& imb (sts.image_binding ());"
       << endl
       << "if (im.version != sts.image_version () || imb.version == 0)"
       << "{"
       << "bind (imb.bind, im" << (versioned ? ", svm" : "") << ");"
       << "sts.image_version (im.version);"
       << "imb.version++;"
       << "}";

    os << "pq.query.init_parameters ();"
       << "st->execute ();";

    post_query_ (c, false);

    os << endl
       << "return shared_ptr<result_impl> (" << endl
       << "new (shared) " << db << "::view_result_impl<view_type> (" << endl
       << "pq.query, st, sts, " << (versioned ? "&svm" : "0") << "));"
       << "}";
  }

  // Generate function table registration for dynamic multi-database
  // support.
  //
  if (multi_dynamic)
  {
    string fn (flat_name (type));
    string dt ("access::view_traits_impl< " + type + ", id_common >");

    os << "static const" << endl
       << dt << "::" << endl
       << "function_table_type function_table_" << fn << "_ ="
       << "{";

    if (!options.omit_unprepared ())
      os << "&" << traits << "::query";

    if (options.generate_prepared ())
    {
      if (!options.omit_unprepared ())
        os << "," << endl;

      os << "&" << traits << "::prepare_query" << "," << endl
         << "&" << traits << "::execute_query";
    }

    os << "};";

    os << "static const view_function_table_entry< " << type << ", " <<
      "id_" << db << " >" << endl
       << "function_table_entry_" << fn << "_ (" << endl
       << "&function_table_" << fn << "_);"
       << endl;
  }
}

namespace relational
{
  namespace source
  {
    static inline void
    add_space (string& s)
    {
      string::size_type n (s.size ());
      if (n != 0 && s[n - 1] != ' ')
        s += ' ';
    }

    static string
    translate_name_trailer (cxx_lexer& l,
                            cpp_ttype& tt,
                            string& tl,
                            tree& tn,
                            cpp_ttype& ptt)
    {
      string r;

      for (; tt != CPP_EOF; ptt = tt, tt = l.next (tl, &tn))
      {
        bool done (false);

        switch (tt)
        {
        case CPP_SCOPE:
        case CPP_DOT:
          {
            r += cxx_lexer::token_spelling[tt];
            break;
          }
        default:
          {
            // Handle CPP_KEYWORD here to avoid a warning (it is not
            // part of the cpp_ttype enumeration).
            //
            if (tt == CPP_NAME || tt == CPP_KEYWORD)
            {
              // For names like 'foo::template bar'.
              //
              if (ptt == CPP_NAME || ptt == CPP_KEYWORD)
                r += ' ';

              r += tl;
            }
            else
              done = true;

            break;
          }
        }

        if (done)
          break;
      }

      return r;
    }

    static class_::expression
    translate_name (cxx_lexer& l,
                    cpp_ttype& tt,
                    string& tl,
                    tree& tn,
                    cpp_ttype& ptt,
                    semantics::scope& start_scope,
                    location_t loc,
                    string const& prag,
                    bool check_ptr,
                    view_alias_map const& amap,
                    view_object_map const& omap)
    {
      using semantics::scope;
      using semantics::data_member;
      typedef class_::expression expression;

      bool multi_obj ((amap.size () + omap.size ()) > 1);

      bool fail (false);
      string name;
      string r ("query_columns");
      context& ctx (context::current ());

      // This code is quite similar to view_data_members in the type
      // processor.
      //
      try
      {
        data_member* m (0);
        view_object* vo (0);

        // Check if this is an alias.
        //
        if (tt == CPP_NAME)
        {
          view_alias_map::const_iterator i (amap.find (tl));

          if (i != amap.end ())
          {
            if (multi_obj)
            {
              r += "::";
              r += i->first;
            }

            vo = i->second;
            fail = true; // This must be a data member.

            // Skip '::'.
            //
            ptt = tt;
            tt = l.next (tl, &tn);

            if (tt != CPP_SCOPE)
            {
              error (loc) << "member name expected after an alias in db " <<
                "pragma " << prag << endl;
              throw operation_failed ();
            }

            ptt = tt;
            if (l.next (tl, &tn) != CPP_NAME)
              throw lookup::invalid_name ();

            m = &vo->obj->lookup<data_member> (tl, scope::include_hidden);

            tt = l.next (tl, &tn);
          }
        }

        // If it is not an alias, do the normal lookup.
        //
        if (vo == 0)
        {
          // Also get the object type. We need to do it so that
          // we can get the correct (derived) object name (the
          // member itself can come from a base class).
          //
          scope* s;
          cpp_ttype ptt; // Not used.
          m = &lookup::resolve_scoped_name<data_member> (
            l, tt, tl, tn, ptt,
            start_scope,
            name,
            false,
            &s);

          view_object_map::const_iterator i (
            omap.find (dynamic_cast<semantics::class_*> (s)));

          if (i == omap.end ())
          {
            // Not an object associated with this view. Assume it
            // is some other valid name.
            //
            return expression (
              name + translate_name_trailer (l, tt, tl, tn, ptt));
          }

          vo = i->second;

          if (multi_obj)
          {
            r += "::";
            r += context::class_name (*vo->obj);
          }
        }

        expression e (vo);
        r += "::";
        r += ctx.public_name (*m);

        // Assemble the member path if we may need to return a pointer
        // expression.
        //
        if (check_ptr)
          e.member_path.push_back (m);

        fail = true; // Now we definitely fail if anything goes wrong.

        // Finally, resolve nested members if any.
        //
        for (; tt == CPP_DOT; ptt = tt, tt = l.next (tl, &tn))
        {
          // Check if this member is actually of a composite value type.
          // This is to handle expressions like "object::member.is_null ()"
          // correctly. The remaining issue here is that in the future
          // is_null()/is_not_null() will be valid for composite values
          // as well.
          //
          semantics::class_* comp (
            context::composite_wrapper (context::utype (*m)));
          if (comp == 0)
            break;

          ptt = tt;
          tt = l.next (tl, &tn);

          if (tt != CPP_NAME)
          {
            error (loc) << "name expected after '.' in db pragma " <<
              prag << endl;
            throw operation_failed ();
          }

          m = &comp->lookup<data_member> (tl, scope::include_hidden);

          r += '.';
          r += ctx.public_name (*m);

          if (check_ptr)
            e.member_path.push_back (m);
        }

        // If requested, check if this member is a pointer. We only do this
        // if there is nothing after this name.
        //
        if (check_ptr && tt == CPP_EOF)
        {
          using semantics::type;

          type* t;

          if (context::container (*m))
            t = &context::container_vt (*m);
          else
            t = &context::utype (*m);

          if (context::object_pointer (*t))
            return e;
        }

        // Read the remainder of the expression (e.g., '.is_null ()') if
        // the member is not composite and we bailed out from the above
        // loop.
        //
        if (tt == CPP_DOT)
          r += translate_name_trailer (l, tt, tl, tn, ptt);

        return expression (r);
      }
      catch (lookup::invalid_name const&)
      {
        if (!fail)
          return expression (
            name + translate_name_trailer (l, tt, tl, tn, ptt));

        error (loc) << "invalid name in db pragma " << prag << endl;
        throw operation_failed ();
      }
      catch (semantics::unresolved const& e)
      {
        if (!fail)
          return expression (
            name + translate_name_trailer (l, tt, tl, tn, ptt));

        if (e.type_mismatch)
          error (loc) << "name '" << e.name << "' in db pragma " << prag <<
            " does not refer to a data member" << endl;
        else
          error (loc) << "unable to resolve data member '" << e.name <<
            "' specified with db pragma " << prag << endl;

        throw operation_failed ();
      }
      catch (semantics::ambiguous const& e)
      {
        error (loc) << "data member name '" << e.first.name () << "' " <<
          "specified with db pragma " << prag << " is ambiguous" << endl;

        info (e.first.named ().location ()) << "could resolve to this " <<
          "data member" << endl;

        info (e.second.named ().location ()) << "or could resolve to this " <<
          "data member" << endl;

        throw operation_failed ();
      }
    }

    class_::expression class_::
    translate_expression (type& c,
                          cxx_tokens const& ts,
                          semantics::scope& scope,
                          location_t loc,
                          string const& prag,
                          bool* placeholder,
                          bool predicate)
    {
      // This code is similar to translate() from context.cxx.
      //

      // The overall idea is as folows: read in tokens and add them
      // to the string. If a token starts a name, try to resolve it
      // to an object member (taking into account aliases). If this
      // was successful, translate it to the query column reference.
      // Otherwise, output it as is.
      //
      // If the placeholder argument is not NULL, then we need to
      // detect the special '(?)' token sequence and replace it
      // with the query variable ('q').
      //
      expression e ("");
      string& r (e.value);

      view_alias_map const& amap (c.get<view_alias_map> ("alias-map"));
      view_object_map const& omap (c.get<view_object_map> ("object-map"));

      cxx_tokens_lexer l;
      l.start (ts);

      tree tn;
      string tl;
      for (cpp_ttype tt (l.next (tl, &tn)), ptt (CPP_EOF); tt != CPP_EOF;)
      {
        // Try to format the expression to resemble the style of the
        // generated code.
        //
        switch (tt)
        {
        case CPP_NOT:
          {
            add_space (r);
            r += '!';
            break;
          }
        case CPP_COMMA:
          {
            r += ", ";
            break;
          }
        case CPP_OPEN_PAREN:
          {
            if (ptt == CPP_NAME ||
                ptt == CPP_KEYWORD)
              add_space (r);

            r += '(';
            break;
          }
        case CPP_CLOSE_PAREN:
          {
            r += ')';
            break;
          }
        case CPP_OPEN_SQUARE:
          {
            r += '[';
            break;
          }
        case CPP_CLOSE_SQUARE:
          {
            r += ']';
            break;
          }
        case CPP_OPEN_BRACE:
          {
            add_space (r);
            r += "{ ";
            break;
          }
        case CPP_CLOSE_BRACE:
          {
            add_space (r);
            r += '}';
            break;
          }
        case CPP_SEMICOLON:
          {
            r += ';';
            break;
          }
        case CPP_ELLIPSIS:
          {
            add_space (r);
            r += "...";
            break;
          }
        case CPP_PLUS:
        case CPP_MINUS:
          {
            bool unary (ptt != CPP_NAME &&
                        ptt != CPP_SCOPE &&
                        ptt != CPP_NUMBER &&
                        ptt != CPP_STRING &&
                        ptt != CPP_CLOSE_PAREN &&
                        ptt != CPP_PLUS_PLUS &&
                        ptt != CPP_MINUS_MINUS);

            if (!unary)
              add_space (r);

            r += cxx_lexer::token_spelling[tt];

            if (!unary)
              r += ' ';
            break;
          }
        case CPP_PLUS_PLUS:
        case CPP_MINUS_MINUS:
          {
            if (ptt != CPP_NAME &&
                ptt != CPP_CLOSE_PAREN &&
                ptt != CPP_CLOSE_SQUARE)
              add_space (r);

            r += cxx_lexer::token_spelling[tt];
            break;
          }
        case CPP_DEREF:
        case CPP_DEREF_STAR:
        case CPP_DOT:
        case CPP_DOT_STAR:
          {
            r += cxx_lexer::token_spelling[tt];
            break;
          }
        case CPP_STRING:
          {
            if (ptt == CPP_NAME ||
                ptt == CPP_KEYWORD ||
                ptt == CPP_STRING ||
                ptt == CPP_NUMBER)
              add_space (r);

            r += strlit (tl);
            break;
          }
        case CPP_NUMBER:
          {
            if (ptt == CPP_NAME ||
                ptt == CPP_KEYWORD ||
                ptt == CPP_STRING ||
                ptt == CPP_NUMBER)
              add_space (r);

            r += tl;
            break;
          }
        case CPP_SCOPE:
        case CPP_NAME:
          {
            // Start of a name.
            //
            if (ptt == CPP_NAME ||
                ptt == CPP_KEYWORD ||
                ptt == CPP_STRING ||
                ptt == CPP_NUMBER)
              add_space (r);

            // Check if this is a pointer expression.
            //
            // If r is not empty, then it means this is not just the
            // name. If placeholder is not 0, then we are translating
            // a query expression, not a join condition.
            //
            expression e (
              translate_name (
                l, tt, tl, tn, ptt,
                scope, loc, prag,
                r.empty () && placeholder == 0, amap, omap));

            if (e.kind == expression::literal)
              r += e.value;
            else
              return e;

            continue; // We have already extracted the next token.
          }
        case CPP_QUERY:
          {
            if (placeholder != 0 && !*placeholder)
            {
              if (ptt == CPP_OPEN_PAREN)
              {
                // Get the next token and see if it is ')'.
                //
                ptt = tt;
                tt = l.next (tl, &tn);

                if (tt == CPP_CLOSE_PAREN)
                {
                  *placeholder = true;

                  // Predicate is true if this is a SELECT statement clause.
                  // Otherwise it is a stored procedure parameters.
                  //
                  if (predicate)
                    r +=  "q.empty () ? query_base_type::true_expr : q";
                  else
                  {
                    r.resize (r.size () - 1); // Remove opening paren.
                    r += "q";
                    break; // Skip the closing paren as well.
                  }
                }
                else
                {
                  // The same as in the default case.
                  //
                  add_space (r);
                  r += "? ";
                }
                continue; // We have already gotten the next token.
              }
            }
          }
          // Fall through.
        default:
          {
            // Handle CPP_KEYWORD here to avoid a warning (it is not
            // part of the cpp_ttype enumeration).
            //
            if (tt == CPP_KEYWORD)
            {
              if (ptt == CPP_NAME ||
                  ptt == CPP_KEYWORD ||
                  ptt == CPP_STRING ||
                  ptt == CPP_NUMBER)
                add_space (r);

              r += tl;
            }
            else
            {
              // All the other operators.
              //
              add_space (r);
              r += cxx_lexer::token_spelling[tt];
              r += ' ';
            }
            break;
          }
        }

        //
        // Watch out for the continue statements above if you add any
        // logic here.
        //

        ptt = tt;
        tt = l.next (tl, &tn);
      }

      return e;
    }

    void
    generate ()
    {
      context ctx;
      ostream& os (ctx.os);

      traversal::unit unit;
      traversal::defines unit_defines;
      typedefs unit_typedefs (false);
      traversal::namespace_ ns;
      instance<class_> c;

      unit >> unit_defines >> ns;
      unit_defines >> c;
      unit >> unit_typedefs >> c;

      traversal::defines ns_defines;
      typedefs ns_typedefs (false);

      ns >> ns_defines >> ns;
      ns_defines >> c;
      ns >> ns_typedefs >> c;

      instance<include> i;
      i->generate ();

      os << "namespace odb"
         << "{";

      unit.dispatch (ctx.unit);

      os << "}";
    }
  }
}
