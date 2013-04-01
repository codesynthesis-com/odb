// file      : odb/relational/source.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

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

  data_member* id (id_member (c));
  bool auto_id (id && auto_ (*id));
  bool base_id (id && &id->scope () != &c); // Comes from base.
  member_access* id_ma (id ? &id->get<member_access> ("get") : 0);

  data_member* opt (optimistic (c));
  member_access* opt_ma_get (opt ? &opt->get<member_access> ("get") : 0);
  member_access* opt_ma_set (opt ? &opt->get<member_access> ("set") : 0);

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
    grow_id = (id ? context::grow (*id) : false) ||
      (opt ? context::grow (*opt) : false);
  }

  string const& type (class_fq_name (c));
  string traits ("access::object_traits_impl< " + type + ", id_" +
                 db.string () + " >");
  column_count_type const& cc (column_count (c));

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

  //
  // Containers (abstract and concrete).
  //
  bool containers (has_a (c, test_container));
  bool straight_containers (false);
  bool straight_readwrite_containers (false);
  bool smart_containers (false);

  if (containers)
  {
    size_t scn (has_a (c, test_straight_container));

    if (scn != 0)
    {
      straight_containers = true;

      // Inverse containers cannot be marked readonly.
      //
      straight_readwrite_containers = scn > has_a (c, test_readonly_container);

      // Inverse containers cannot be smart.
      //
      smart_containers = has_a (c, test_smart_container);
    }
  }

  if (containers)
  {
    instance<container_traits> t (c);
    t->traverse (c);
  }

  //
  // Functions (abstract and concrete).
  //

  // id (image_type)
  //
  if (!poly_derived && id != 0 && !base_id)
  {
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
      init_id_value_member_->traverse (*id);
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

  // discriminator()
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
       << "grow (image_type& i, " << truncated_vector << " t";

    if (poly_derived)
      os << ", std::size_t d";

    os << ")"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (i);"
       << "ODB_POTENTIALLY_UNUSED (t);"
       << endl
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
  // we get the the external id binding.
  //
  if (poly_derived)
    os << "const " << bind_vector << " id," << endl
       << "std::size_t id_size," << endl;

  os << "image_type& i," << endl
     << db << "::statement_kind sk)"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (sk);"
     << endl
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
    os << "// " << id->name () << endl
       << "//" << endl
       << "if (sk == statement_insert)"
       << "{"
       << "if (id != 0)" << endl
       << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
       << "n += id_size;"
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
      os << "// " << id->name () << endl
         << "//" << endl
         << "if (sk == statement_update)"
         << "{"
         << "if (id != 0)" << endl
         << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
         << "n += id_size;"
         << "}";

    // Bind the image chain for the select statement. Seeing that
    // this is the last statement in the function, we don't care
    // about updating n.
    //
    os << "// " << class_name (*poly_base) << " base" << endl
       << "//" << endl
       << "if (sk == statement_select)" << endl
       << "base_traits::";

    if (poly_base == poly_root)
      os << "bind (b + n, *i.base, sk);";
    else
      os << "bind (b + n, id, id_size, *i.base, sk);";
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

    bind_id_member_->traverse (*id);

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
     << "init (image_type& i, const object_type& o, " <<
    db << "::statement_kind sk)"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (i);"
     << "ODB_POTENTIALLY_UNUSED (o);"
     << "ODB_POTENTIALLY_UNUSED (sk);"
     << endl
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
     << "init (object_type& o, const image_type& i, database* db";

  if (poly_derived)
    os << ", std::size_t d";

  os << ")"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (o);"
     << "ODB_POTENTIALLY_UNUSED (i);"
     << "ODB_POTENTIALLY_UNUSED (db);"
     << endl;

  if (poly_derived)
  {
    os << "// " << class_name (*poly_base) << " base" << endl
       << "//" << endl
       << "if (--d != 0)" << endl
       << "base_traits::init (o, *i.base, db" <<
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

    init_id_image_member_->traverse (*id);

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

  // Statement cache (definition).
  //
  if (id != 0)
  {
    os << "struct " << traits << "::container_statement_cache_type"
       << "{";

    instance<container_cache_members> cm;
    cm->traverse (c);

    os << (containers ? "\n" : "")
       << "container_statement_cache_type (" << endl
       << db << "::connection&" << (containers ? " c" : "") << "," << endl
       << db << "::binding&" << (containers ? " id" : "");

    container_cache_extra_args (containers);

    os << ")";

    instance<container_cache_init_members> im;
    im->traverse (c);

    os << "{"
       << "}"
       << "};";
  }

  // Polymorphic map.
  //
  if (poly)
  {
    if (!poly_derived)
      os << traits << "::map_type*" << endl
         << traits << "::map;"
         << endl;

    os << "const " << traits << "::" << (abst ? "abstract_" : "") <<
      "info_type" << endl
       << traits << "::info (" << endl
       << "typeid (" << type << ")," << endl;

    if (poly_derived)
      os << "&object_traits_impl< " << class_fq_name (*poly_base) <<
        ", id_" << db << " >::info";
    else
      os << "0";

    string n;

    if (!abst)
    {
      n = class_fq_name (c);

      os << "," << endl
         << strlit (string (n, 2, string::npos)) << "," << endl
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
         << "polymorphic_entry_for_" << flat_name (n) << ";"
         << endl;
  }

  //
  // Statements.
  //

  qname table (table_name (c));
  string qtable (quote_id (table));

  // persist_statement
  //
  {
    statement_columns sc;
    {
      statement_kind sk (statement_insert); // Imperfect forwarding.
      instance<object_columns> ct (sk, sc);
      ct->traverse (c);
      process_statement_columns (sc, statement_insert);
    }

    bool dv (sc.empty ()); // The DEFAULT VALUES syntax.

    os << "const char " << traits << "::persist_statement[] =" << endl
       << strlit ("INSERT INTO " + qtable + (dv ? "" : " (")) << endl;

    for (statement_columns::const_iterator i (sc.begin ()),
           e (sc.end ()); i != e;)
    {
      string const& c (i->column);
      os << strlit (c + (++i != e ? "," : ")")) << endl;
    }

    instance<query_parameters> qp (table);

    persist_statement_extra (c, *qp, persist_after_columns);

    if (!dv)
    {
      string values;
      instance<persist_statement_params> pt (values, *qp);
      pt->traverse (c);
      os << strlit (" VALUES (" + values + ")");
    }
    else
      os << strlit (" DEFAULT VALUES");

    persist_statement_extra (c, *qp, persist_after_values);

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
        instance<object_columns> t (qtable, sk, sc, d);
        t->traverse (c);
        process_statement_columns (sc, statement_select);
        find_column_counts[poly_depth - d] = sc.size ();
      }

      if (sc.size () != 0)
      {
        os << strlit ("SELECT ") << endl;

        for (statement_columns::const_iterator i (sc.begin ()),
               e (sc.end ()); i != e;)
        {
          string const& c (i->column);
          os << strlit (c + (++i != e ? "," : "")) << endl;
        }

        os << strlit (" FROM " + qtable) << endl;

        if (d != 1)
        {
          size_t d1 (d - 1); //@@ (im)perfect forward.
          instance<polymorphic_object_joins> j (c, d1);
          j->traverse (polymorphic_base (c));
        }

        bool f (false); // @@ (im)perfect forwarding
        instance<object_joins> j (c, f, d); // @@ (im)perfect forwarding
        j->traverse (c);

        instance<query_parameters> qp (table);
        for (object_columns_list::iterator b (id_cols->begin ()), i (b);
             i != id_cols->end (); ++i)
        {
          if (i != b)
            os << endl;

          os << strlit ((i == b ? " WHERE " : " AND ") +
                        qtable + "." + quote_id (i->name) + "=" +
                        convert_to (qp->next (), i->type, *i->member));
        }
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

        process_statement_columns (sc, statement_select);
      }

      os << "const char " << traits << "::" << endl
         << "find_discriminator_statement[] =" << endl
         << strlit ("SELECT ") << endl;

      for (statement_columns::const_iterator i (sc.begin ()),
             e (sc.end ()); i != e;)
      {
        string const& c (i->column);
        os << strlit (c + (++i != e ? "," : "")) << endl;
      }

      os << strlit (" FROM " + qtable) << endl;

      instance<query_parameters> qp (table);
      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        if (i != b)
          os << endl;

        os << strlit ((i == b ? " WHERE " : " AND ") +
                      qtable + "." + quote_id (i->name) + "=" +
                      convert_to (qp->next (), i->type, *i->member));
      }

      os << ";"
         << endl;
    }

    // update_statement
    //
    if (cc.total != cc.id + cc.inverse + cc.readonly)
    {
      instance<query_parameters> qp (table);

      statement_columns sc;
      {
        query_parameters* p (qp.get ()); // Imperfect forwarding.
        statement_kind sk (statement_update); // Imperfect forwarding.
        instance<object_columns> t (sk, sc, p);
        t->traverse (c);
        process_statement_columns (sc, statement_update);
      }

      os << "const char " << traits << "::update_statement[] =" << endl
         << strlit ("UPDATE " + qtable + " SET ") << endl;

      for (statement_columns::const_iterator i (sc.begin ()),
             e (sc.end ()); i != e;)
      {
        string const& c (i->column);
        os << strlit (c + (++i != e ? "," : "")) << endl;
      }

      update_statement_extra (c);

      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        if (i != b)
          os << endl;

        os << strlit ((i == b ? " WHERE " : " AND ") +
                      quote_id (i->name) + "=" +
                      convert_to (qp->next (), i->type, *i->member));
      }

      if (opt != 0 && !poly_derived)
      {
        // Top-level version column.
        //
        os << endl
           << strlit (" AND " + column_qname (*opt, column_prefix ()) + "=" +
                      convert_to (qp->next (), *opt));
      }

      os << ";"
         << endl;
    }

    // erase_statement
    //
    {
      instance<query_parameters> qp (table);
      os << "const char " << traits << "::erase_statement[] =" << endl
         << strlit ("DELETE FROM " + qtable);

      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {

        os << endl
           << strlit ((i == b ? " WHERE " : " AND ") +
                      quote_id (i->name) + "=" +
                      convert_to (qp->next (), i->type, *i->member));
      }

      os << ";"
         << endl;
    }

    if (opt != 0 && !poly_derived)
    {
      instance<query_parameters> qp (table);

      os << "const char " << traits << "::optimistic_erase_statement[] " <<
        "=" << endl
         << strlit ("DELETE FROM " + qtable);

      for (object_columns_list::iterator b (id_cols->begin ()), i (b);
           i != id_cols->end (); ++i)
      {
        os << endl
           << strlit ((i == b ? " WHERE " : " AND ") +
                      quote_id (i->name) + "=" +
                      convert_to (qp->next (), i->type, *i->member));
      }

      // Top-level version column.
      //
      os << endl
         << strlit (" AND " + column_qname (*opt, column_prefix ()) + "=" +
                    convert_to (qp->next (), *opt)) << ";"
         << endl;
    }
  }

  if (options.generate_query ())
  {
    // query_statement
    //
    statement_columns sc;
    {
      statement_kind sk (statement_select); // Imperfect forwarding.
      instance<object_columns> oc (qtable, sk, sc, poly_depth);
      oc->traverse (c);
      process_statement_columns (sc, statement_select);
    }

    os << "const char " << traits << "::query_statement[] =" << endl
       << strlit ("SELECT ") << endl;

    for (statement_columns::const_iterator i (sc.begin ()),
           e (sc.end ()); i != e;)
    {
      string const& c (i->column);
      os << strlit (c + (++i != e ? "," : "")) << endl;
    }

    os << strlit (" FROM " + qtable) << endl;

    if (poly_depth != 1)
    {
      size_t d (poly_depth - 1); //@@ (im)perfect forward.
      instance<polymorphic_object_joins> j (c, d);
      j->traverse (polymorphic_base (c));
    }

    if (id != 0)
    {
      bool t (true); //@@ (im)perfect forwarding
      instance<object_joins> oj (c, t, poly_depth); //@@ (im)perfect forwarding
      oj->traverse (c);
    }

    os << strlit (" ") << ";"
       << endl;

    // erase_query_statement
    //
    os << "const char " << traits << "::erase_query_statement[] =" << endl
       << strlit ("DELETE FROM " + qtable) << endl;

    // DELETE JOIN:
    //
    // MySQL:
    // << strlit ("DELETE FROM " + qtable + " USING " + qtable) << endl;
    // << strlit ("DELETE " + qtable + " FROM " + qtable) << endl;
    // oj->write ();
    //

    os << strlit (" ") << ";"
       << endl;

    // table_name
    //
    os << "const char " << traits << "::table_name[] =" << endl
       << strlit (qtable) << ";" // Use quoted name.
       << endl;
  }

  // persist ()
  //
  os << "void " << traits << "::" << endl
     << "persist (database& db, " << (auto_id ? "" : "const ") <<
    "object_type& obj";

  if (poly)
    os << ", bool top, bool dyn";

  os << ")"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (db);";

  if (poly)
    os << "ODB_POTENTIALLY_UNUSED (top);";

  os << endl
     << "using namespace " << db << ";"
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
     << db << "::transaction::current ().connection ());"
     << "statements_type& sts (" << endl
     << "conn.statement_cache ().find_object<object_type> ());"
     << endl;

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

  os << "init (im, obj, statement_insert)";

  if (generate_grow)
    os << ")" << endl
       << "im.version++";

  os << ";"
     << endl;

  if (!poly_derived && auto_id && insert_send_auto_id)
  {
    string const& n (id->name ());
    string var ("im." + n + (n[n.size () - 1] == '_' ? "" : "_"));
    init_auto_id (*id, var);
    os << endl;
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

  os << "im, statement_insert);";

  if (poly_derived)
    os << "sts.insert_id_binding_version (idb.version);";

  os << "sts.insert_image_version (im.version);"
     << "imb.version++;"
     << "}";

  os << "insert_statement& st (sts.persist_statement ());"
     << "if (!st.execute ())" << endl
     << "throw object_already_persistent ();"
     << endl;

  if (!poly_derived && auto_id)
  {
    member_access& ma (id->get<member_access> ("set"));

    if (!ma.synthesized)
      os << "// From " << location_string (ma.loc, true) << endl;

    if (ma.placeholder ())
      os << ma.translate ("obj", "static_cast< id_type > (st.id ())") << ";"
         << endl;
    else
    {
      // If this member is const and we have a synthesized direct access,
      // then cast away constness. Otherwise, we assume that the user-
      // provided expression handles this.
      //
      bool cast (ma.direct () && const_type (id->type ()));
      if (cast)
        os << "const_cast< id_type& > (" << endl;

      os << ma.translate ("obj");

      if (cast)
        os << ")";

      os << " = static_cast< id_type > (st.id ());"
         << endl;
    }
  }

  // Set the optimistic concurrency version in the object member.
  //
  if (opt != 0 && !poly_derived)
  {
    // If we don't have auto id, then obj is a const reference.
    //
    string obj (auto_id ? "obj" : "const_cast< object_type& > (obj)");
    string init (optimimistic_version_init (*opt));

    if (!opt_ma_set->synthesized)
      os << "// From " << location_string (opt_ma_set->loc, true) << endl;

    if (opt_ma_set->placeholder ())
      os << opt_ma_set->translate (obj, init) << ";"
         << endl;
    else
    {
      // If this member is const and we have a synthesized direct access,
      // then cast away constness. Otherwise, we assume that the user-
      // provided expression handles this.
      //
      bool cast (opt_ma_set->direct () && const_type (opt->type ()));
      if (cast)
        os << "const_cast< version_type& > (" << endl;

      os << opt_ma_set->translate (obj);

      if (cast)
        os << ")";

      os << " = " << init << ";"
         << endl;
    }
  }

  // Initialize id_image and binding if we are a root of a polymorphic
  // hierarchy or if we have non-inverse containers.
  //
  if (!poly_derived && (poly || straight_containers))
  {
    // If this is a polymorphic root without containers, then we only
    // need to do this if we are not a top-level call. If we are poly-
    // abstract, then top will always be false.
    //
    if (poly && !straight_containers && !abst)
      os << "if (!top)"
         << "{";

    os << "id_image_type& i (sts.id_image ());";

    if (!id_ma->synthesized)
      os << "// From " << location_string (id_ma->loc, true) << endl;

    os << "init (i, " <<  id_ma->translate ("obj") << ");"
       << endl
       << "binding& idb (sts.id_image_binding ());"
       << "if (i.version != sts.id_image_version () || idb.version == 0)"
       << "{"
       << "bind (idb.bind, i);"
       << "sts.id_image_version (i.version);"
       << "idb.version++;"
       << "}";

    if (poly && !straight_containers && !abst)
      os << "}";
  }

  if (straight_containers)
  {
    instance<container_calls> t (container_calls::persist_call);
    t->traverse (c);
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

  // update ()
  //
  if (id != 0 && (!readonly || poly))
  {
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

      if (poly_derived)
      {
        bool readonly_base (context::readonly (*poly_base));

        if (readonly_base ||
            cc.total != cc.id + cc.inverse + cc.readonly ||
            straight_readwrite_containers)
        {
          os << db << "::connection& conn (" << endl
             << db << "::transaction::current ().connection ());"
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
          // we don't have any columns or containers to update, then we
          // only have to do it if this is not a top-level call. If we
          // are abstract, then top is always false.
          //
          if (cc.total == cc.id + cc.inverse + cc.readonly &&
              !straight_readwrite_containers &&
              !abst)
            os << "if (!top)";

          os << "{"
             << "id_image_type& i (sts.id_image ());";

          if (!id_ma->synthesized)
            os << "// From " << location_string (id_ma->loc, true) << endl;

          os << "init (i, " << id_ma->translate ("obj") << ");"
             << endl;

          os << "binding& idb (sts.id_image_binding ());"
             << "if (i.version != sts.id_image_version () || idb.version == 0)"
             << "{"
             << "bind (idb.bind, i);"
             << "sts.id_image_version (i.version);"
             << "idb.version++;"
             << "}"
             << "}";
        }

        if (cc.total != cc.id + cc.inverse + cc.readonly)
        {
          os << "const binding& idb (sts.id_image_binding ());"
             << endl;
        }

        if (cc.total != cc.id + cc.inverse + cc.readonly)
        {
          // Initialize the object image.
          //
          os << "image_type& im (sts.image ());";

          if (generate_grow)
            os << "if (";

          os << "init (im, obj, statement_update)";

          if (generate_grow)
            os << ")" << endl
               << "im.version++";

          os << ";"
             << endl;

          os << "binding& imb (sts.update_image_binding ());"
             << "if (idb.version != sts.update_id_binding_version () ||" << endl
             << "im.version != sts.update_image_version () ||" << endl
             << "imb.version == 0)"
             << "{"
             << "bind (imb.bind, idb.bind, idb.count, im, statement_update);"
             << "sts.update_id_binding_version (idb.version);"
             << "sts.update_image_version (im.version);"
             << "imb.version++;"
             << "}";

          os << "if (sts.update_statement ().execute () == 0)" << endl
             << "throw object_not_persistent ();"
             << endl;
        }
        else if (readonly_base)
        {
          // If our base is readonly and we don't have any columns to
          // update, then we have to make sure this object actually
          // exists in the database. Use the discriminator_() call for
          // that.
          //
          if (!id_ma->synthesized)
            os << "// From " << location_string (id_ma->loc, true) << endl;

          os << "root_traits::discriminator_ (sts.root_statements (), " <<
            id_ma->translate ("obj") << ", 0);" << endl;
        }
        // Otherwise, nothing else to do here if we don't have any columns
        // to update.
        //
      }
      else if (cc.total != cc.id + cc.inverse + cc.readonly)
      {
        os << db << "::connection& conn (" << endl
           << db << "::transaction::current ().connection ());"
           << "statements_type& sts (" << endl
           << "conn.statement_cache ().find_object<object_type> ());"
           << endl;

        // Initialize object and id images.
        //
        if (opt != 0)
        {
          if (!opt_ma_get->synthesized)
            os << "// From " << location_string (opt_ma_get->loc, true) << endl;

          os << "const version_type& v (" << endl
             << opt_ma_get->translate ("obj") << ");";
        }

        os << "id_image_type& i (sts.id_image ());";

        if (!id_ma->synthesized)
          os << "// From " << location_string (id_ma->loc, true) << endl;

        os << "init (i, " << id_ma->translate ("obj");

        if (opt != 0)
          os << ", &v";

        os << ");"
           << endl
           << "image_type& im (sts.image ());";

        if (generate_grow)
            os << "if (";

          os << "init (im, obj, statement_update)";

          if (generate_grow)
            os << ")" << endl
               << "im.version++";

          os << ";"
             << endl;

        // Update binding is bound to two images (object and id)
        // so we have to track both versions.
        //
        os << "bool u (false);" // Avoid incrementing version twice.
           << "binding& imb (sts.update_image_binding ());"
           << "if (im.version != sts.update_image_version () ||" << endl
           << "imb.version == 0)"
           << "{"
           << "bind (imb.bind, im, statement_update);"
           << "sts.update_image_version (im.version);"
           << "imb.version++;"
           << "u = true;"
           << "}";

        // To update the id part of the update binding we have to do
        // it indirectly via the id binding, which just points to the
        // suffix of the update bind array (see object_statements).
        //
        os << "binding& idb (sts.id_image_binding ());"
           << "if (i.version != sts.update_id_image_version () ||" << endl
           << "idb.version == 0)"
           << "{"
          // If the id binding is up-to-date, then that means update
          // binding is too and we just need to update the versions.
          //
           << "if (i.version != sts.id_image_version () ||" << endl
           << "idb.version == 0)"
           << "{"
           << "bind (idb.bind, i);"
          // Update the id binding versions since we may use them later
          // to update containers.
          //
           << "sts.id_image_version (i.version);"
           << "idb.version++;"
           << "}"
           << "sts.update_id_image_version (i.version);"
           << endl
           << "if (!u)" << endl
           << "imb.version++;"
           << "}";

        os << "if (sts.update_statement ().execute () == 0)" << endl;

        if (opt == 0)
          os << "throw object_not_persistent ();";
        else
          os << "throw object_changed ();";

        os << endl;
      }
      else
      {
        // We don't have any columns to update. Note that we still have
        // to make sure this object exists in the database. For that we
        // will run the SELECT query using the find_() function.
        //
        //
        os << db << "::connection& conn (" << endl
           << db << "::transaction::current ().connection ());"
           << "statements_type& sts (" << endl
           << "conn.statement_cache ().find_object<object_type> ());"
           << endl;

        if (!id_ma->synthesized)
          os << "// From " << location_string (id_ma->loc, true) << endl;

        os << "const id_type& id (" << endl
           << id_ma->translate ("obj") << ");"
           << endl;

        if (poly)
        {
          // In case of a polymorphic root, use discriminator_(), which
          // is faster. And initialize the id image, unless this is a
          // top-level call.
          //
          os << "discriminator_ (sts, id, 0);"
             << endl;

          if (!abst)
            os << "if (!top)";

          os << "{"
             << "id_image_type& i (sts.id_image ());";

          os << "init (i, id);"
             << endl;

          os << "binding& idb (sts.id_image_binding ());"
             << "if (i.version != sts.id_image_version () || idb.version == 0)"
             << "{"
             << "bind (idb.bind, i);"
             << "sts.id_image_version (i.version);"
             << "idb.version++;"
             << "}"
             << "}";
        }
        else
        {
          os << "if (!find_ (sts, &id))" << endl
             << "throw object_not_persistent ();"
             << endl;

          if (delay_freeing_statement_result)
            os << "sts.find_statement ().free_result ();";
        }
      }

      if (straight_readwrite_containers)
      {
        instance<container_calls> t (container_calls::update_call);
        t->traverse (c);
      }

      // Update the optimistic concurrency version in the object member.
      //
      if (opt != 0 && !poly_derived)
      {
        // Object is passed as const reference so we need to cast away
        // constness.
        //
        string obj ("const_cast< object_type& > (obj)");
        string inc (optimimistic_version_increment (*opt));

        if (!opt_ma_set->synthesized)
          os << "// From " << location_string (opt_ma_set->loc, true) << endl;

        if (opt_ma_set->placeholder ())
        {
          if (!opt_ma_get->synthesized)
            os << "// From " << location_string (opt_ma_get->loc, true) <<
              endl;

          if (inc == "1")
            os << opt_ma_set->translate (
              obj, opt_ma_get->translate ("obj") + " + 1") << ";";
          else
            os << opt_ma_set->translate (obj, inc) << ";";

          os << endl;
        }
        else
        {
          // If this member is const and we have a synthesized direct access,
          // then cast away constness. Otherwise, we assume that the user-
          // provided expression handles this.
          //
          bool cast (opt_ma_set->direct () && const_type (opt->type ()));
          if (cast)
            os << "const_cast< version_type& > (" << endl;

          os << opt_ma_set->translate (obj);

          if (cast)
            os << ")";

          if (inc == "1")
            os << "++;";
          else
            os << " = " << inc << ";";

          os << endl;
        }
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

  // erase (id)
  //
  if (id != 0)
  {
    os << "void " << traits << "::" << endl
       << "erase (database& db, const id_type& id";

    if (poly)
      os << ", bool top, bool dyn";

    os << ")"
       << "{"
       << "using namespace " << db << ";"
       << endl
       << "ODB_POTENTIALLY_UNUSED (db);";

    if (poly)
      os << "ODB_POTENTIALLY_UNUSED (top);";

    os << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection ());"
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
         << "idb.version++;"
         << "}";

      if (poly)
        os << "}";
    }

    // Erase containers first so that there are no reference
    // violations (we don't want to rely on ON DELETE CASCADE
    // here since in case of a custom schema, it might not be
    // there).
    //
    if (straight_containers)
    {
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

  // erase (object)
  //
  if (id != 0 && (poly || opt != 0 || smart_containers))
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

    if (opt != 0 || smart_containers)
    {
      string rsts (poly_derived ? "rsts" : "sts");

      os << "using namespace " << db << ";"
         << endl
         << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection ());"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (poly_derived)
        os << "root_statements_type& rsts (sts.root_statements ());";

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

      if (!abst || straight_containers)
      {
        if (!id_ma->synthesized)
          os << "// From " << location_string (id_ma->loc, true) << endl;

        os << "const id_type& id  (" << endl
           << id_ma->translate ("obj") << ");"
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
             << "}";

          if (poly)
            os << "}";
        }

        // Erase containers first so that there are no reference
        // violations (we don't want to rely on ON DELETE CASCADE
        // here since in case of a custom schema, it might not be
        // there).
        //
        instance<container_calls> t (container_calls::erase_obj_call);
        t->traverse (c);

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

          if (!opt_ma_get->synthesized)
            os << "// From " << location_string (opt_ma_get->loc, true) << endl;

          os << "const version_type& v (" << endl
             << opt_ma_get->translate ("obj") << ");"
             << "id_image_type& i (" << rsts << ".id_image ());"
             << "init (i, id, &v);"
             << endl;

          // To update the id part of the optimistic id binding we have
          // to do it indirectly via the id binding, since both id and
          // optimistic id bindings just point to the suffix of the
          // update bind array (see object_statements).
          //
          os << "binding& oidb (" << rsts << ".optimistic_id_image_binding ());"
             << "if (i.version != " << rsts <<
            ".optimistic_id_image_version () ||" << endl
             << "oidb.version == 0)"
             << "{"
            // If the id binding is up-to-date, then that means optimistic
            // id binding is too and we just need to update the versions.
            //
             << "binding& idb (" << rsts << ".id_image_binding ());"
             << "if (i.version != " << rsts << ".id_image_version () ||" << endl
             << "idb.version == 0)"
             << "{"
             << "bind (idb.bind, i);"
            // Update the id binding versions since we may use them later
            // to delete containers.
            //
             << rsts << ".id_image_version (i.version);"
             << "idb.version++;"
             << "}"
             << rsts << ".optimistic_id_image_version (i.version);"
             << "oidb.version++;"
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
        if (poly_derived || (poly && straight_containers))
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

            if (!opt_ma_get->synthesized)
              os << "// From " << location_string (opt_ma_get->loc, true) << endl;

            os << "if (v != " << opt_ma_get->translate ("obj") << ")" << endl
               << "throw object_changed ();"
               << "}";
          }
        }
        else if (straight_containers)
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
          os << "if (!find_ (sts, &id))" << endl
             << "throw object_changed ();"
             << endl;

          if (delay_freeing_statement_result)
            os << "sts.find_statement ().free_result ();"
               << endl;

          if (!opt_ma_get->synthesized)
            os << "// From " << location_string (opt_ma_get->loc, true) << endl;

          os << "if (version (sts.image ()) != " <<
            opt_ma_get->translate ("obj") << ")" << endl
             << "throw object_changed ();"
             << endl;
        }

        // Erase containers first so that there are no reference
        // violations (we don't want to rely on ON DELETE CASCADE
        // here since in case of a custom schema, it might not be
        // there).
        //
        if (straight_containers)
        {
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
    else if (smart_containers)
    {

    }
    else
    {
      os << "callback (db, obj, callback_event::pre_erase);"
         << "erase (db, id (obj), true, false);"
         << "callback (db, obj, callback_event::post_erase);";
    }

    os << "}";
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

    // Get the connection.
    //
    os << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection ());"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());";

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
       << "if (!find_ (sts, &id))" << endl
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

    os << "init (obj, sts.image (), &db);";

    init_value_extra ();

    if (delay_freeing_statement_result)
      os << "ar.free ();";

    os << "load_ (sts, obj);";

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

    os << rsts << ".load_delayed ();"
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
         << db << "::transaction::current ().connection ());"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (poly_derived)
        os << "root_statements_type& rsts (sts.root_statements ());";

      // This can only be top-level call so auto_lock must succeed.
      //
      os << endl
         << "statements_type::auto_lock l (" << rsts << ");"
         << endl;

      os << "if (!find_ (sts, &id))" << endl
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
         << "init (obj, sts.image (), &db);";

      init_value_extra ();

      if (delay_freeing_statement_result)
        os << "ar.free ();";

      os << "load_ (sts, obj);"
         << rsts << ".load_delayed ();"
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
         << db << "::transaction::current ().connection ());"
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());";

      if (poly_derived)
        os << "root_statements_type& rsts (sts.root_statements ());";

      // This can only be top-level call so auto_lock must succeed.
      //
      os << endl
         << "statements_type::auto_lock l (" << rsts << ");"
         << endl;

      if (!id_ma->synthesized)
        os << "// From " << location_string (id_ma->loc, true) << endl;

      os << "const id_type& id  (" << endl
         << id_ma->translate ("obj") << ");"
         << endl;

      os << "if (!find_ (sts, &id))" << endl
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
        if (!opt_ma_get->synthesized)
          os << "// From " << location_string (opt_ma_get->loc, true) << endl;

        os << "if (" << (poly_derived ? "root_traits::" : "") << "version (" <<
          rsts << ".image ()) == " << opt_ma_get->translate ("obj") <<
          ")" << endl
           << "return true;"
           << endl;
      }

      os << "callback (db, obj, callback_event::pre_load);"
         << "init (obj, sts.image (), &db);";

      init_value_extra ();

      if (delay_freeing_statement_result)
        os << "ar.free ();";

      os << "load_ (sts, obj);"
         << rsts << ".load_delayed ();"
         << "l.unlock ();"
         << "callback (db, obj, callback_event::post_load);"
         << "return true;";
    }

    os << "}";
  }

  // find_ ()
  //
  if (id != 0)
  {
    os << "bool " << traits << "::" << endl
       << "find_ (";

    if (poly && !poly_derived)
      os << "base_statements_type& sts, ";
    else
      os << "statements_type& sts, ";

    os << "const id_type* id";

    if (poly_derived && !abst)
      os << ", std::size_t d";

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
       << "idb.version++;"
       << "}";

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
         << "bind (imb.bind, 0, 0, im, statement_select);"
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
         << "bind (imb.bind, im, statement_select);"
         << "sts.select_image_version (im.version);"
         << "imb.version++;"
         << "}";
    }

    os << "select_statement& st (sts.find_statement (" <<
      (poly_derived ? (abst ? "depth" : "d") : "") << "));"
       << "st.execute ();"
       << "auto_result ar (st);"
       << "select_statement::result r (st.fetch ());"
       << endl;

    if (grow)
    {
      os << "if (r == select_statement::truncated)"
         << "{"
         << "if (grow (im, sts.select_image_truncated ()" <<
        (poly_derived ? (abst ? ", depth" : ", d") : "") << "))" << endl
         << "im.version++;"
         << endl;

      if (poly_derived)
      {
        os << "if (check_version (sts.select_image_versions (), im))"
           << "{"
           << "bind (imb.bind, 0, 0, im, statement_select);"
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
           << "bind (imb.bind, im, statement_select);"
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

  // load_() containers
  //
  if (containers || poly_derived)
  {
    os << "void " << traits << "::" << endl
       << "load_ (";

    if (poly && !poly_derived)
      os << "base_statements_type& sts, ";
    else
      os << "statements_type& sts, ";

    os << "object_type& obj";

    if (poly_derived)
      os << ", std::size_t d";

    os << ")"
       << "{";

    if (poly_derived)
      os << "if (--d != 0)" << endl
         << "base_traits::load_ (sts.base_statements (), obj" <<
        (poly_base != poly_root ? ", d" : "") << ");"
         << endl;

    if (containers)
    {
      instance<container_calls> t (container_calls::load_call);
      t->traverse (c);
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
       << db << "::transaction::current ().connection ());"
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_object<object_type> ());"
       << endl
       << "d = depth - d;" // Convert to distance from derived.
       << endl;

    // Avoid trying to execute an empty SELECT statement.
    //
    if (empty_depth != 0)
      os << "if (d > " << (poly_depth - empty_depth) << "UL)"
         << "{";

    os << "if (!find_ (sts, 0, d))" << endl
       << "throw object_not_persistent ();" // Database inconsistency.
       << endl;

    os << "select_statement& st (sts.find_statement (d));"
       << "ODB_POTENTIALLY_UNUSED (st);"
       << endl;

    if (delay_freeing_statement_result)
      os << "auto_result ar (st);";

    os << "init (obj, sts.image (), &db, d);";

    init_value_extra ();

    if (delay_freeing_statement_result)
      os << "ar.free ();";

    if (empty_depth != 0)
      os << "}";

    os << "load_ (sts, obj, d);"
       << "}";
  }

  // discriminator_ ()
  //
  if (poly && !poly_derived)
  {
    os << "void " << traits << "::" << endl
       << "discriminator_ (statements_type & sts," << endl
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

    // Unprepared.
    //
    if (!options.omit_unprepared ())
    {
      // query ()
      //
      os << "result< " << traits << "::object_type >" << endl
         << traits << "::" << endl
         << "query (database&, const query_base_type& q)"
         << "{"
         << "using namespace " << db << ";"
         << "using odb::details::shared;"
         << "using odb::details::shared_ptr;"
         << endl
         << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection ());"
         << endl
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());"
         << endl;

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
           << "bind (imb.bind, 0, 0, im, statement_select);"
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
           << "bind (imb.bind, im, statement_select);"
           << "sts.select_image_version (im.version);"
           << "imb.version++;"
           << "}";
      }

      os << "q.init_parameters ();"
         << "shared_ptr<select_statement> st (" << endl
         << "new (shared) select_statement (" << endl;
      object_query_statement_ctor_args (c, "q", false);
      os << "));" << endl
         << "st->execute ();";

      post_query_ (c, true);

      os << endl
         << "shared_ptr< odb::" << result_type << " > r (" << endl
         << "new (shared) " << db << "::" << result_type << " (" << endl
         << "q, st, sts));"
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
       << "erase_query (database&, const query_base_type& q)"
       << "{"
       << "using namespace " << db << ";"
       << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection ());"
       << endl
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
         << endl
         << db << "::connection& conn (" << endl
         << "static_cast<" << db << "::connection&> (c));"
         << endl
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());"
         << endl;

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
           << "bind (imb.bind, 0, 0, im, statement_select);"
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
           << "bind (imb.bind, im, statement_select);"
           << "sts.select_image_version (im.version);"
           << "imb.version++;"
           << "}";
      }

      os << "shared_ptr<" << db << "::prepared_query_impl> r (" << endl
         << "new (shared) " << db << "::prepared_query_impl (conn));"
         << "r->name = n;"
         << "r->execute = &execute_query;"
         << "r->query = q;"
         << "r->stmt.reset (" << endl
         << "new (shared) select_statement (" << endl;
      object_query_statement_ctor_args (c, "r->query", true);
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
         << endl
         << db << "::connection& conn (" << endl
         << db << "::transaction::current ().connection ());"
         << endl
         << "// The connection used by the current transaction and the" << endl
         << "// one used to prepare this statement must be the same." << endl
         << "//" << endl
         << "assert (&conn == &st->connection ());"
         << endl
         << "statements_type& sts (" << endl
         << "conn.statement_cache ().find_object<object_type> ());"
         << endl;

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
           << "bind (imb.bind, 0, 0, im, statement_select);"
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
           << "bind (imb.bind, im, statement_select);"
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
         << "pq.query, st, sts));"
         << "}";
    }
  }

  // Generate embedded schema.
  //
  if (embedded_schema)
    schema_->traverse (c);

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
  string const& type (class_fq_name (c));
  string traits ("access::view_traits_impl< " + type + ", id_" +
                 db.string () + " >");

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
  if (generate_grow)
  {
    os << "bool " << traits << "::" << endl
       << "grow (image_type& i, " << truncated_vector << " t)"
       << "{"
       << "ODB_POTENTIALLY_UNUSED (i);"
       << "ODB_POTENTIALLY_UNUSED (t);"
       << endl
       << "bool grew (false);"
       << endl;

    index_ = 0;
    names (c, grow_member_names_);

    os << "return grew;"
       << "}";
  }

  // bind (image_type)
  //
  os << "void " << traits << "::" << endl
     << "bind (" << bind_vector << " b, image_type& i)"
     << "{"
     << "using namespace " << db << ";"
     << endl
     << db << "::statement_kind sk (statement_select);"
     << "ODB_POTENTIALLY_UNUSED (sk);"
     << endl
     << "std::size_t n (0);"
     << endl;

  names (c, bind_member_names_);

  os << "}";

  // init (view, image)
  //
  os << "void " << traits << "::" << endl
     << "init (view_type& o, const image_type& i, database* db)"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (o);"
     << "ODB_POTENTIALLY_UNUSED (i);"
     << "ODB_POTENTIALLY_UNUSED (db);"
     << endl;

  names (c, init_value_member_names_);

  os << "}";

  // query_statement()
  //
  view_query& vq (c.get<view_query> ("query"));

  if (vq.kind != view_query::runtime)
  {
    os << traits << "::query_base_type" << endl
       << traits << "::" << endl
       << "query_statement (const query_base_type& q)"
       << "{";

    if (vq.kind == view_query::complete)
    {
      os << "query_base_type r (" << endl;

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
          os << strlit (string (vq.literal, 0, p + 1)) << " +" << endl
             << "(q.empty () ? query_base_type::true_expr : q) +" << endl
             << strlit (string (vq.literal, p + 2));
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
             c, vq.expr, scope, vq.loc, "query", &ph).value;
      }

      os << ");";

      // If there was no placeholder, add the query condition
      // at the end.
      //
      if (!ph)
        os << "r += q.clause_prefix ();"
           << "r += q;";
    }
    else // vq.kind == view_query::condition
    {
      statement_columns sc;
      {
        instance<view_columns> t (sc);
        t->traverse (c);
        process_statement_columns (sc, statement_select);
      }

      os << "query_base_type r (" << endl
         << strlit ("SELECT ") << endl;

      for (statement_columns::const_iterator i (sc.begin ()),
             e (sc.end ()); i != e;)
      {
        string const& c (i->column);
        os << strlit (c + (++i != e ? "," : "")) << endl;
      }

      os << ");"
         << endl;

      // Generate from-list.
      //
      view_objects const& objs (c.get<view_objects> ("objects"));

      for (view_objects::const_iterator i (objs.begin ());
           i != objs.end ();
           ++i)
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

            os << "r += " << strlit (l) << ";"
               << endl;

            continue;
          }

          l = "LEFT JOIN ";
          l += quote_id (i->tbl_name);

          if (!i->alias.empty ())
            l += (need_alias_as ? " AS " : " ") + quote_id (i->alias);

          semantics::scope& scope (
            dynamic_cast<semantics::scope&> (*unit.find (i->scope)));

          expression e (
            translate_expression (
              c, i->cond, scope, i->loc, "table"));

          if (e.kind != expression::literal)
          {
            error (i->loc)
              << "invalid join condition in db pragma table" << endl;

            throw operation_failed ();
          }

          l += " ON";

          // Output the pragma location for easier error tracking.
          //
          os << "r += " << strlit (l) << ";"
             << "// From " << location_string (i->loc, true) << endl
             << "r += " << e.value << ";"
             << endl;

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

          os << "r += " << strlit (l) << ";";

          if (poly_depth != 1)
          {
            size_t d (poly_depth - 1); //@@ (im)perfect forward.
            instance<polymorphic_object_joins> j (
              o, d, i->alias, "r += ", ";");
            j->traverse (polymorphic_base (o));
          }

          os << endl;

          continue;
        }

        semantics::scope& scope (
          dynamic_cast<semantics::scope&> (*unit.find (i->scope)));

        expression e (
          translate_expression (
            c, i->cond, scope, i->loc, "object"));

        // Literal expression.
        //
        if (e.kind == expression::literal)
        {
          l = "LEFT JOIN ";
          l += table_qname (o);

          if (!alias.empty ())
            l += (need_alias_as ? " AS " : " ") + quote_id (alias);

          l += " ON";

          // Output the pragma location for easier error tracking.
          //
          os << "r += " << strlit (l) << ";"
             << "// From " << location_string (i->loc, true) << endl
             << "r += " << e.value << ";";

          if (poly_depth != 1)
          {
            size_t d (poly_depth - 1); //@@ (im)perfect forward.
            instance<polymorphic_object_joins> j (
              o, d, i->alias, "r += ", ";");
            j->traverse (polymorphic_base (o));
          }

          os << endl;

          continue;
        }

        // We have an object relationship (pointer) for which we need
        // to come up with the corresponding JOIN condition. If this
        // is a to-many relationship, then we first need to JOIN the
        // container table. This code is similar to object_joins.
        //
        using semantics::data_member;

        data_member& m (*e.member_path.back ());

        // Resolve the pointed-to object to view_object and do
        // some sanity checks while at it.
        //
        semantics::class_* c (0);

        if (semantics::type* cont = container (m))
          c = object_pointer (container_vt (*cont));
        else
          c = object_pointer (utype (m));

        view_object const* vo (0);

        // Check if the pointed-to object has been previously
        // associated with this view and is unambiguous. A
        // pointer to ourselves is always assumed to point
        // to this association.
        //
        if (&o == c)
          vo = &*i;
        else
        {
          bool ambig (false);

          for (view_objects::const_iterator j (objs.begin ());
               j != i;
               ++j)
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
              error (i->loc)
                << "pointed-to object '" << class_name (*c) <<  "' is "
                << "ambiguous" << endl;

              info (i->loc)
                << "candidates are:" << endl;

              info (vo->loc)
                << "  '" << vo->name () << "'" << endl;

              ambig = true;
            }

            info (j->loc)
              << "  '" << j->name () << "'" << endl;
          }

          if (ambig)
          {
            info (i->loc)
              << "use the full join condition clause in db pragma "
              << "object to resolve this ambiguity" << endl;

            throw operation_failed ();
          }

          if (vo == 0)
          {
            error (i->loc)
              << "pointed-to object '" << class_name (*c) << "' "
              << "specified in the join condition has not been "
              << "previously associated with this view" << endl;

            throw operation_failed ();
          }
        }

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
        data_member* im (inverse (m));

        semantics::type* cont (container (im != 0 ? *im : m));

        string ct; // Container table.
        if (cont != 0)
        {
          if (im != 0)
          {
            // For now a direct member can only be directly in
            // the object scope. If this changes, the inverse()
            // function would have to return a member path instead
            // of just a single member.
            //
            ct = table_qname (*im, table_prefix (*vo->obj));
          }
          else
            ct = table_qname (*e.vo->obj, e.member_path);

          l = "LEFT JOIN ";
          l += ct;
          l += " ON";
          os << "r += " << strlit (l) << ";";

          // If we are the pointed-to object, then we have to turn
          // things around. This is necessary to have the proper
          // JOIN order. There seems to be a pattern there but it
          // is not yet intuitively clear what it means.
          //
          instance<object_columns_list> c_cols; // Container columns.
          instance<object_columns_list> o_cols; // Object columns.

          qname* ot; // Object table (either lt or rt).

          if (im != 0)
          {
            if (&o == c)
            {
              // container.value = pointer.id
              //
              semantics::data_member& id (*id_member (*e.vo->obj));

              c_cols->traverse (*im, utype (id), "value", "value");
              o_cols->traverse (id);
              ot = &lt;
            }
            else
            {
              // container.id = pointed-to.id
              //
              semantics::data_member& id (*id_member (*vo->obj));

              c_cols->traverse (
                *im, utype (id), "id", "object_id", vo->obj);
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
              semantics::data_member& id (*id_member (*e.vo->obj));

              c_cols->traverse (
                m, utype (id), "id", "object_id", e.vo->obj);
              o_cols->traverse (id);
              ot = &lt;
            }
            else
            {
              // container.value = pointed-to.id
              //
              semantics::data_member& id (*id_member (*vo->obj));

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

            os << "r += " << strlit (l) << ";";
          }
        }

        l = "LEFT JOIN ";
        l += table_qname (o);

        if (!alias.empty ())
          l += (need_alias_as ? " AS " : " ") + quote_id (alias);

        l += " ON";
        os << "r += " << strlit (l) << ";";

        if (cont != 0)
        {
          instance<object_columns_list> c_cols; // Container columns.
          instance<object_columns_list> o_cols; // Object columns.

          qname* ot; // Object table (either lt or rt).

          if (im != 0)
          {
            if (&o == c)
            {
              // container.id = pointed-to.id
              //
              semantics::data_member& id (*id_member (*vo->obj));

              c_cols->traverse (*im, utype (id), "id", "object_id", vo->obj);
              o_cols->traverse (id);
              ot = &rt;
            }
            else
            {
              // container.value = pointer.id
              //
              semantics::data_member& id (*id_member (*e.vo->obj));

              c_cols->traverse (*im, utype (id), "value", "value");
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
              semantics::data_member& id (*id_member (*vo->obj));

              c_cols->traverse (m, utype (id), "value", "value");
              o_cols->traverse (id);
              ot = &rt;
            }
            else
            {
              // container.id = pointer.id
              //
              semantics::data_member& id (*id_member (*e.vo->obj));

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

            os << "r += " << strlit (l) << ";";
          }
        }
        else
        {
          column_prefix col_prefix;

          if (im == 0)
            col_prefix = column_prefix (e.member_path);

          instance<object_columns_list> l_cols (col_prefix);
          instance<object_columns_list> r_cols;

          if (im != 0)
          {
            // our.id = pointed-to.pointer
            //
            l_cols->traverse (*id_member (*e.vo->obj));
            r_cols->traverse (*im);
          }
          else
          {
            // our.pointer = pointed-to.id
            //
            l_cols->traverse (*e.member_path.back ());
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

            os << "r += " << strlit (l) << ";";
          }
        }

        if (poly_depth != 1)
        {
          size_t d (poly_depth - 1); //@@ (im)perfect forward.
          instance<polymorphic_object_joins> j (
            o, d, i->alias, "r += ", ";");
          j->traverse (polymorphic_base (o));
        }

        os << endl;

      } // End JOIN-generating for-loop.

      // Generate the query condition.
      //
      if (!vq.literal.empty () || !vq.expr.empty ())
      {
        os << "query_base_type c (" << endl;

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
            os << "c.optimize ();";
        }

        if (!ph)
          os << "c += q;";

        os << "r += c.clause_prefix ();"
           << "r += c;"
           << endl;
      }
      else
      {
        os << "r += q.clause_prefix ();"
           << "r += q;"
           << endl;
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
       << "query (database&, const query_base_type& q)"
       << "{"
       << "using namespace " << db << ";"
       << "using odb::details::shared;"
       << "using odb::details::shared_ptr;"
       << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection ());"
       << endl
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_view<view_type> ());"
       << endl
       << "image_type& im (sts.image ());"
       << "binding& imb (sts.image_binding ());"
       << endl
       << "if (im.version != sts.image_version () || imb.version == 0)"
       << "{"
       << "bind (imb.bind, im);"
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
    view_query_statement_ctor_args (c, "qs", false);
    os << "));" << endl
       << "st->execute ();";

    post_query_ (c, true);

    os << endl
       << "shared_ptr< odb::view_result_impl<view_type> > r (" << endl
       << "new (shared) " << db << "::view_result_impl<view_type> (" << endl
       << "qs, st, sts));"
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
       << endl
       << db << "::connection& conn (" << endl
       << "static_cast<" << db << "::connection&> (c));"
       << endl
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_view<view_type> ());"
       << endl;

    // Rebind the image if necessary.
    //
    os << "image_type& im (sts.image ());"
       << "binding& imb (sts.image_binding ());"
       << endl
       << "if (im.version != sts.image_version () || imb.version == 0)"
       << "{"
       << "bind (imb.bind, im);"
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
    view_query_statement_ctor_args (c, "r->query", true);
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
       << endl
       << db << "::connection& conn (" << endl
       << db << "::transaction::current ().connection ());"
       << endl
       << "// The connection used by the current transaction and the" << endl
       << "// one used to prepare this statement must be the same." << endl
       << "//" << endl
       << "assert (&conn == &st->connection ());"
       << endl
       << "statements_type& sts (" << endl
       << "conn.statement_cache ().find_view<view_type> ());"
       << endl;

    // Rebind the image if necessary.
    //
    os << "image_type& im (sts.image ());"
       << "binding& imb (sts.image_binding ());"
       << endl
       << "if (im.version != sts.image_version () || imb.version == 0)"
       << "{"
       << "bind (imb.bind, im);"
       << "sts.image_version (im.version);"
       << "imb.version++;"
       << "}";

    os << "pq.query.init_parameters ();"
       << "st->execute ();";

    post_query_ (c, false);

    os << endl
       << "return shared_ptr<result_impl> (" << endl
       << "new (shared) " << db << "::view_result_impl<view_type> (" << endl
       << "pq.query, st, sts));"
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

          if (type* c = context::container (*m))
            t = &context::container_vt (*c);
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
                          bool* placeholder)
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
                  r +=  "q.empty () ? query_base_type::true_expr : q";
                  *placeholder = true;
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
            // Fall through.
          }
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
