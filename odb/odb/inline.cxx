// file      : odb/inline.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/common.hxx>
#include <odb/context.hxx>
#include <odb/generate.hxx>
#include <odb/diagnostics.hxx>

using namespace std;

namespace inline_
{
  //
  //
  struct callback_calls: traversal::class_, virtual context
  {
    callback_calls ()
    {
      *this >> inherits_ >> *this;
    }

    void
    traverse (type& c, bool constant)
    {
      const_ = constant;
      traverse (c);
    }

    virtual void
    traverse (type& c)
    {
      bool obj (object (c));

      // Ignore transient bases.
      //
      if (!(obj || view (c)))
        return;

      if (c.count ("callback"))
      {
        string name (c.get<string> ("callback"));

        // In case of the const instance, we only generate the call if
        // there is a const callback. Note also that we cannot use
        // object_type/view_type alias because it can be a base type.
        //
        string const& type (class_fq_name (c));

        if (const_)
        {
          if (c.count ("callback-const"))
            os << "static_cast<const " << type << "&> (x)." << name <<
              " (e, db);";
        }
        else
          os << "static_cast< " << type << "&> (x)." << name << " (e, db);";
      }
      else if (obj)
        inherits (c);
    }

  protected:
    bool const_;
    traversal::inherits inherits_;
  };

  struct class_: traversal::class_, virtual context
  {
    class_ ()
        : typedefs_ (false)
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

    callback_calls callback_calls_;
  };
}

void inline_::class_::
traverse_object (type& c)
{
  using semantics::data_member;

  data_member_path* id (id_member (c));
  data_member* idf (id ? id->front () : 0);
  bool auto_id (id && auto_ (*id));
  bool base_id (id && &idf->scope () != &c); // Comes from base.

  data_member* opt (context::optimistic (c));

  // Base class that contains the object id.
  //
  type* base (base_id ? dynamic_cast<type*> (&idf->scope ()) : 0);

  bool poly (polymorphic (c));
  bool abst (abstract (c));
  bool reuse_abst (abst && !poly);

  user_sections& uss (c.get<user_sections> ("user-sections"));

  string const& type (class_fq_name (c));
  string traits ("access::object_traits< " + type + " >");

  os << "// " << class_name (c) << endl
     << "//" << endl
     << endl;

  // id (object_type)
  //
  if (id != 0 || !reuse_abst)
  {
    os << "inline" << endl
       << traits << "::id_type" << endl
       << traits << "::" << endl
       << "id (const object_type&" << (id != 0 ? " o" : "") << ")"
       << "{";

    if (id != 0)
    {
      if (base_id)
        os << "return object_traits< " << class_fq_name (*base) <<
          " >::id (o);";
      else
      {
        // Get the id using the accessor expressions.
        //
        string r ("o");

        for (data_member_path::const_iterator b (id->begin ()), i (b);
             i != id->end ();
             ++i)
        {
          member_access& ma ((*i)->get<member_access> ("get"));

          // If this is not a synthesized expression, then output its
          // location for easier error tracking.
          //
          if (!ma.synthesized)
            os << "// From " << location_string (ma.loc, true) << endl;

          r = ma.translate (r);
        }

        os << "return " << r << ";";
      }
    }

    os << "}";
  }

  if (opt != 0)
  {
    os << "inline" << endl
       << traits << "::version_type" << endl
       << traits << "::" << endl
       << "version (const object_type& o)"
       << "{";

    if (base_id)
      os << "return object_traits< " << class_fq_name (*base) <<
        " >::version (o);";
      else
      {
        // Get the id using the accessor expression. If this is not
        // a synthesized expression, then output its location for
        // easier error tracking.
        //
        member_access& ma (opt->get<member_access> ("get"));

        if (!ma.synthesized)
          os << "// From " << location_string (ma.loc, true) << endl;

        os << "return " << ma.translate ("o") << ";";
      }

    os << "}";
  }

  // The rest does not apply to reuse-abstract objects.
  //
  if (reuse_abst)
    return;

  // callback ()
  //
  os << "inline" << endl
     << "void " << traits << "::" << endl
     << "callback (database& db, object_type& x, callback_event e)"
     <<  endl
     << "{"
     << "ODB_POTENTIALLY_UNUSED (db);"
     << "ODB_POTENTIALLY_UNUSED (x);"
     << "ODB_POTENTIALLY_UNUSED (e);"
     << endl;
  callback_calls_.traverse (c, false);
  os << "}";

  os << "inline" << endl
     << "void " << traits << "::" << endl
     << "callback (database& db, const object_type& x, callback_event e)"
     << "{"
     << "ODB_POTENTIALLY_UNUSED (db);"
     << "ODB_POTENTIALLY_UNUSED (x);"
     << "ODB_POTENTIALLY_UNUSED (e);"
     << endl;
  callback_calls_.traverse (c, true);
  os << "}";

  // The rest only applies to dynamic milti-database support.
  //
  if (!multi_dynamic)
    return;

  traits = "access::object_traits_impl< " + type + ", id_common >";

  //
  // Forwarding functions.
  //

  // persist ()
  //
  os << "inline" << endl
     << "void " << traits << "::" << endl
     << "persist (database& db, " << (auto_id ? "" : "const ") <<
    "object_type& o)"
     << "{"
     << "function_table[db.id ()]->persist (db, o" <<
    (poly ? ", true, true" : "") << ");"
     << "}";

  if (id != 0)
  {
    // find (id)
    //
    if (c.default_ctor ())
    {
      os << "inline" << endl
         << traits << "::pointer_type" << endl
         << traits << "::" << endl
         << "find (database& db, const id_type& id)"
         << "{"
         << "return function_table[db.id ()]->find1 (db, id);"
         << "}";
    }

    // find (id, obj)
    //
    os << "inline" << endl
       << "bool " << traits << "::" << endl
       << "find (database& db, const id_type& id, object_type& o)"
       << "{"
       << "return function_table[db.id ()]->find2 (db, id, o" <<
      (poly ? ", true" : "") << ");"
       << "}";

    // reload ()
    //
    os << "inline" << endl
       << "bool " << traits << "::" << endl
       << "reload (database& db, object_type& o)"
       << "{"
       << "return function_table[db.id ()]->reload (db, o" <<
      (poly ? ", true" : "") << ");"
       << "}";

    // update ()
    //
    // In case of a polymorphic object, we generate update() even if it is
    // readonly since the potentially-readwrite base will rely on it to
    // initialize the id image.
    //
    //
    if (!readonly (c) || poly)
    {
      os << "inline" << endl
         << "void " << traits << "::" << endl
         << "update (database& db, const object_type& o)"
         << "{"
         << "function_table[db.id ()]->update (db, o" <<
        (poly ? ", true, true" : "") << ");"
         << "}";
    }

    // erase ()
    //
    os << "inline" << endl
       << "void " << traits << "::" << endl
       << "erase (database& db, const id_type& id)"
       << "{"
       << "function_table[db.id ()]->erase1 (db, id" <<
      (poly ? ", true, true" : "") << ");"
       << "}";

    os << "inline" << endl
       << "void " << traits << "::" << endl
       << "erase (database& db, const object_type& o)"
       << "{"
       << "function_table[db.id ()]->erase2 (db, o" <<
      (poly ? ", true, true" : "") << ");"
       << "}";

    // Sections.
    //
    if (uss.count (user_sections::count_total |
                   user_sections::count_load  |
                   (poly ? user_sections::count_load_empty : 0)) != 0)
      os << "inline" << endl
         << "bool " << traits << "::" << endl
         << "load (connection& c, object_type& o, section& s)"
         << "{"
         << "return function_table[c.database ().id ()]->load_section (" <<
        "c, o, s" << (poly ? ", 0" : "") << ");"
         << "}";

    if (uss.count (user_sections::count_total  |
                   user_sections::count_update |
                   (poly ? user_sections::count_update_empty : 0)) != 0)
      os << "inline" << endl
         << "bool " << traits << "::" << endl
         << "update (connection& c, const object_type& o, const section& s)"
         << "{"
         << "return function_table[c.database ().id ()]->update_section (" <<
        "c, o, s" << (poly ? ", 0" : "") << ");"
         << "}";
  }

  if (options.generate_query ())
  {
    if (!options.omit_unprepared ())
    {
      os << "inline" << endl
         << "result< " << traits << "::object_type >" << endl
         << traits << "::" << endl
         << "query (database& db, const query_base_type& q)"
         << "{"
         << "return function_table[db.id ()]->query (db, q);"
         << "}";
    }

    os << "inline" << endl
       << "unsigned long long " << traits << "::" << endl
       << "erase_query (database& db, const query_base_type& q)"
       << "{"
       << "return function_table[db.id ()]->erase_query (db, q);"
       << "}";

    if (options.generate_prepared ())
    {
      os << "inline" << endl
         << "odb::details::shared_ptr<prepared_query_impl>" << endl
         << traits << "::" << endl
         << "prepare_query (connection& c, const char* n, " <<
        "const query_base_type& q)"
         << "{"
         << "return function_table[c.database ().id ()]->prepare_query (" <<
        "c, n, q);"
         << "}";

      os << "inline" << endl
         << "odb::details::shared_ptr<result_impl>" << endl
         << traits << "::" << endl
         << "execute_query (prepared_query_impl& pq)"
         << "{"
         << "return function_table[pq.conn.database ().id ()]->" <<
        "execute_query (pq);"
         << "}";
    }
  }
}

void inline_::class_::
traverse_view (type& c)
{
  string const& type (class_fq_name (c));
  string traits ("access::view_traits< " + type + " >");

  os << "// " << class_name (c) << endl
     << "//" << endl
     << endl;

  // callback ()
  //
  os << "inline" << endl
     << "void " << traits << "::" << endl
     << "callback (database& db, view_type& x, callback_event e)"
     <<  endl
     << "{"
     << "ODB_POTENTIALLY_UNUSED (db);"
     << "ODB_POTENTIALLY_UNUSED (x);"
     << "ODB_POTENTIALLY_UNUSED (e);"
     << endl;
  callback_calls_.traverse (c, false);
  os << "}";

  // The rest only applies to dynamic milti-database support.
  //
  if (!multi_dynamic)
    return;

  traits = "access::view_traits_impl< " + type + ", id_common >";

  //
  // Forwarding functions.
  //

  if (!options.omit_unprepared ())
  {
    os << "inline" << endl
       << "result< " << traits << "::view_type >" << endl
       << traits << "::" << endl
       << "query (database& db, const query_base_type& q)"
       << "{"
       << "return function_table[db.id ()]->query (db, q);"
       << "}";
  }

  if (options.generate_prepared ())
  {
    os << "inline" << endl
       << "odb::details::shared_ptr<prepared_query_impl>" << endl
       << traits << "::" << endl
       << "prepare_query (connection& c, const char* n, " <<
      "const query_base_type& q)"
       << "{"
       << "return function_table[c.database ().id ()]->prepare_query (" <<
      "c, n, q);"
       << "}";

    os << "inline" << endl
       << "odb::details::shared_ptr<result_impl>" << endl
       << traits << "::" << endl
       << "execute_query (prepared_query_impl& pq)"
       << "{"
       << "return function_table[pq.conn.database ().id ()]->" <<
      "execute_query (pq);"
       << "}";
  }
}

namespace inline_
{
  void
  generate ()
  {
    context ctx;
    ostream& os (ctx.os);

    if (ctx.multi_dynamic)
      os << "#include <odb/database.hxx>" << endl
         << endl;

    traversal::unit unit;
    traversal::defines unit_defines;
    typedefs unit_typedefs (false);
    traversal::namespace_ ns;
    class_ c;

    unit >> unit_defines >> ns;
    unit_defines >> c;
    unit >> unit_typedefs >> c;

    traversal::defines ns_defines;
    typedefs ns_typedefs (false);

    ns >> ns_defines >> ns;
    ns_defines >> c;
    ns >> ns_typedefs >> c;

    os << "namespace odb"
       << "{";

    unit.dispatch (ctx.unit);

    os << "}";
  }
}
