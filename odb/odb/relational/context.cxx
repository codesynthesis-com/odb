// file      : odb/relational/context.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>

#include <odb/relational/context.hxx>

using namespace std;

namespace relational
{
  context* context::current_;

  context::
  ~context ()
  {
    if (current_ == this)
      current_ = 0;
  }

  context::
  context ()
      : data_ (current ().data_),
        model (current ().model),
        generate_grow (current ().generate_grow),
        need_alias_as (current ().need_alias_as),
        insert_send_auto_id (current ().insert_send_auto_id),
        delay_freeing_statement_result (current ().delay_freeing_statement_result),
        need_image_clone (current ().need_image_clone),
        generate_bulk (current ().generate_bulk),
        global_index (current ().global_index),
        global_fkey (current ().global_fkey),
        bind_vector (data_->bind_vector_),
        truncated_vector (data_->truncated_vector_)
  {
  }

  context::
  context (data* d, sema_rel::model* m)
      : data_ (d),
        model (m),
        bind_vector (data_->bind_vector_),
        truncated_vector (data_->truncated_vector_)
  {
    assert (current_ == 0);
    current_ = this;
  }

  string context::
  index_name (qname const& table, string const& base)
  {
    string n;

    if (options.index_suffix ().count (db) != 0)
      n = base + options.index_suffix ()[db];
    else
      n = compose_name (base, "i");

    // If this database has global index names, then add the table
    // name as a prefix (the schema, if needed, will be added by
    // database-specific create_index overrides).
    //
    if (global_index)
      n = compose_name (table.uname (), n);

    return transform_name (n, sql_name_index);
  }

  string context::
  fkey_name (qname const& table, string const& base)
  {
    string n;

    if (options.fkey_suffix ().count (db) != 0)
      n = base + options.fkey_suffix ()[db];
    else
      n = compose_name (base, "fk");

    // If this database has global index names, then add the table
    // name as a prefix (the schema, if needed, will be added by
    // database-specific create_foreign_key overrides).
    //
    if (global_fkey)
      n = compose_name (table.uname (), n);

    return transform_name (n, sql_name_fkey);
  }

  string context::
  convert (string const& e, string const& c)
  {
    size_t p (c.find ("(?)"));
    string r (c, 0, p);
    r += e;
    r.append (c, p + 3, string::npos);
    return r;
  }

  string const& context::
  convert_expr (string const&, semantics::data_member&, bool)
  {
    assert (false);
    throw operation_failed ();
  }

  bool context::
  grow_impl (semantics::class_&, user_section*)
  {
    return false;
  }

  bool context::
  grow_impl (semantics::data_member&)
  {
    return false;
  }

  bool context::
  grow_impl (semantics::data_member&,
             semantics::type&,
             const custom_cxx_type*,
             string const&)
  {
    return false;
  }

  string context::
  quote_string_impl (string const& s) const
  {
    string r;
    r.reserve (s.size ());
    r += '\'';

    for (string::size_type i (0), n (s.size ()); i < n; ++i)
    {
      if (s[i] == '\'')
        r += "''";
      else
        r += s[i];
    }

    r += '\'';
    return r;
  }

  string context::
  quote_id_impl (qname const& id) const
  {
    string r;

    bool f (true);
    for (qname::iterator i (id.begin ()); i < id.end (); ++i)
    {
      if (i->empty ())
        continue;

      if (f)
        f = false;
      else
        r += '.';

      r += '"';
      r += *i;
      r += '"';
    }

    return r;
  }
}
