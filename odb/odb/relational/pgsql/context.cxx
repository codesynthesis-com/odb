// file      : odb/relational/pgsql/context.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>
#include <sstream>

#include <odb/diagnostics.hxx>

#include <odb/sql-token.hxx>
#include <odb/sql-lexer.hxx>

#include <odb/relational/pgsql/context.hxx>
#include <odb/relational/pgsql/common.hxx>

using namespace std;

namespace relational
{
  namespace pgsql
  {
    namespace
    {
      struct type_map_entry
      {
        char const* const cxx_type;
        char const* const db_type;
        char const* const db_id_type;
        bool const null;
      };

      type_map_entry type_map[] =
      {
        {"bool", "BOOLEAN", 0, false},

        {"char", "CHAR(1)", 0, false},
        {"signed char", "SMALLINT", 0, false},
        {"unsigned char", "SMALLINT", 0, false},

        {"short int", "SMALLINT", 0, false},
        {"short unsigned int", "SMALLINT", 0, false},

        {"int", "INTEGER", 0, false},
        {"unsigned int", "INTEGER", 0, false},

        {"long int", "BIGINT", 0, false},
        {"long unsigned int", "BIGINT", 0, false},

        {"long long int", "BIGINT", 0, false},
        {"long long unsigned int", "BIGINT", 0, false},

        {"float", "REAL", 0, false},
        {"double", "DOUBLE PRECISION", 0, false},

        {"::std::string", "TEXT", 0, false},

        {"::size_t", "BIGINT", 0, false},
        {"::std::size_t", "BIGINT", 0, false}
      };
    }

    context* context::current_;

    context::
    ~context ()
    {
      if (current_ == this)
        current_ = 0;
    }

    context::
    context (ostream& os,
             semantics::unit& u,
             options_type const& ops,
             features_type& f,
             sema_rel::model* m)
        : root_context (os, u, ops, f, data_ptr (new (shared) data (os))),
          base_context (static_cast<data*> (root_context::data_.get ()), m),
          data_ (static_cast<data*> (base_context::data_))
    {
      assert (current_ == 0);
      current_ = this;

      generate_grow = true;
      need_alias_as = true;
      insert_send_auto_id = false;
      delay_freeing_statement_result = false;
      need_image_clone = false;
      generate_bulk = true;
      global_index = true;
      global_fkey = false;
      data_->bind_vector_ = "pgsql::bind*";
      data_->truncated_vector_ = "bool*";

      // Populate the C++ type to DB type map.
      //
      for (size_t i (0); i < sizeof (type_map) / sizeof (type_map_entry); ++i)
      {
        type_map_entry const& e (type_map[i]);

        type_map_type::value_type v (
          e.cxx_type,
          db_type_type (
            e.db_type, e.db_id_type ? e.db_id_type : e.db_type, e.null));

        data_->type_map_.insert (v);
      }
    }

    context::
    context ()
        : data_ (current ().data_)
    {
    }

    namespace
    {
      struct has_grow: traversal::class_
      {
        has_grow (bool& r, user_section* s)
            : r_ (r), section_ (s)
        {
          *this >> inherits_ >> *this;
        }

        virtual void
        traverse (type& c)
        {
          // Ignore transient bases.
          //
          if (!(context::object (c) || context::composite (c)))
            return;

          if (section_ == 0 && c.count ("pgsql-grow"))
            r_ = c.get<bool> ("pgsql-grow");
          else
          {
            // r_ should be false.
            //
            inherits (c);

            if (!r_)
              names (c);

            if (section_ == 0)
              c.set ("pgsql-grow", r_);
          }
        }

      private:
        bool& r_;
        user_section* section_;
        traversal::inherits inherits_;
      };

      struct has_grow_member: member_base
      {
        has_grow_member (bool& r, user_section* section = 0)
            : relational::member_base (0, 0, string (), string (), section),
              r_ (r) {}

        has_grow_member (bool& r,
                         user_section* section,
                         semantics::type* t,
                         const custom_cxx_type* ct,
                         string const& key_prefix = string ())
            : relational::member_base (t, ct, string (), key_prefix, section),
              r_ (r) {}

        virtual bool
        pre (member_info& mi)
        {
          // If we have a key prefix (container), then it can't be in a
          // section (while mi.m can). The same for top-level -- if we got
          // called, then we shouldn't ignore it.
          //
          return !key_prefix_.empty () || top_level_ ||
            (section_ == 0 && !separate_load (mi.m)) ||
            (section_ != 0 && *section_ == section (mi.m));
        }

        virtual void
        traverse_composite (member_info& mi)
        {
          // By calling grow() instead of recursing, we reset any overrides.
          // We also don't pass section since they don't apply inside
          // composites.
          //
          r_ = r_ || context::grow (dynamic_cast<semantics::class_&> (mi.t));
        }

        virtual void
        traverse_numeric (member_info&)
        {
          r_ = true;
        }

        virtual void
        traverse_string (member_info&)
        {
          r_ = true;
        }

        virtual void
        traverse_varbit (member_info&)
        {
          r_ = true;
        }

      private:
        bool& r_;
      };
    }

    bool context::
    grow_impl (semantics::class_& c, user_section* section)
    {
      if (section == 0 && c.count ("pgsql-grow"))
        return c.get<bool> ("pgsql-grow");

      bool r (false);
      has_grow ct (r, section);
      has_grow_member mt  (r, section);
      traversal::names names;
      ct >> names >> mt;
      ct.traverse (c);
      return r;
    }

    bool context::
    grow_impl (semantics::data_member& m)
    {
      bool r (false);
      has_grow_member mt  (r);
      mt.traverse (m, true);
      return r;
    }

    bool context::
    grow_impl (semantics::data_member& m,
               semantics::type& t,
               const custom_cxx_type* ct,
               string const& kp)
    {
      bool r (false);
      has_grow_member mt  (r, 0, &t, ct, kp);
      mt.traverse (m, true);
      return r;
    }

    string const& context::
    convert_expr (string const& sqlt, semantics::data_member& m, bool to)
    {
      sql_type const& t (parse_sql_type (sqlt, m));
      return to ? t.to : t.from;
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

        // Warn if the name is greater than the (NAMEDATALEN - 1) limit,
        // which is 63 in the default PG build.
        //
        if (i->size () > 63)
        {
          cerr << "warning: SQL name '" << *i << "' is longer than "
               << "the default PostgreSQL name limit of 63 characters "
               << "and may be truncated" << endl;

          cerr << "info: consider shortening it using #pragma db "
               << "table/column/index or --*-regex options" << endl;
        }

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

    string context::
    statement_name (string const& type, string const& name, semantics::node& n)
    {
      // Put the type first so that in the case of truncation it
      // remains thus lowering the chance of a clash.
      //
      string r (type);
      r += '_';
      r += name;

      r = transform_name (r, sql_name_statement);

      // Warn if the name is greater than the (NAMEDATALEN - 1) limit,
      // which is 63 in the default PG build.
      //
      // Note that we have to do it in addition to the above since this
      // name doesn't go through quote_id().
      //
      if (r.size () > 63)
      {
        location const& l (n.location ());

        warn (l) << "prepared statement name '" << r << "' is longer than "
                 << "the default PostgreSQL name limit of 63 characters "
                 << "and may be truncated" << endl;

        info (l) << "consider shortening the corresponding namespace "
                 << "name, class name, or data member name" << endl;

        info (l) << "or shortening the statement name itself using the "
                 << "--statement-regex option" << endl;
      }

      return r;
    }

    string context::
    database_type_impl (semantics::type& t,
                        semantics::names* hint,
                        bool id,
                        bool* null)
    {
      string r (base_context::database_type_impl (t, hint, id, null));

      if (!r.empty ())
        return r;

      using semantics::array;

      // char[N] mapping.
      //
      if (array* a = dynamic_cast<array*> (&t))
      {
        semantics::type& bt (a->base_type ());
        if (bt.is_a<semantics::fund_char> ())
        {
          unsigned long long n (a->size ());

          if (n == 0)
            return r;
          else if (n == 1)
            r = "CHAR(";
          else
          {
            r = "VARCHAR(";
            n--;
          }

          ostringstream ostr;
          ostr << n;
          r += ostr.str ();
          r += ')';
        }
      }

      return r;
    }

    //
    // SQL type parsing.
    //

    sql_type const& context::
    parse_sql_type (string const& t, semantics::data_member& m, bool custom)
    {
      // If this proves to be too expensive, we can maintain a cache of
      // parsed types across contexts.
      //
      data::sql_type_cache::iterator i (data_->sql_type_cache_.find (t));

      if (i != data_->sql_type_cache_.end ()
          && (custom ? i->second.custom_cached : i->second.straight_cached))
      {
        return (custom ? i->second.custom : i->second.straight);
      }
      else
      {
        try
        {
          sql_type st (
            parse_sql_type (
              t,
              custom ? &unit.get<custom_db_types> ("custom-db-types") : 0));

          if (custom)
            return data_->sql_type_cache_[t].cache_custom (st);
          else
            return data_->sql_type_cache_[t].cache_straight (st);
        }
        catch (invalid_sql_type const& e)
        {
          cerr << m.file () << ":" << m.line () << ":" << m.column ()
               << ": error: " << e.message () << endl;

          throw operation_failed ();
        }
      }
    }

    inline sql_type
    error (bool fail, string const& m)
    {
      if (!fail)
        return sql_type ();
      else
        throw context::invalid_sql_type (m);
    }

    sql_type context::
    parse_sql_type (string sqlt, custom_db_types const* ct)
    {
      try
      {
        sql_type r;

        // First run the type through the custom mapping, if requested.
        //
        if (ct != 0)
        {
          for (custom_db_types::const_iterator i (ct->begin ());
               i != ct->end (); ++i)
          {
            custom_db_type const& t (*i);

            if (t.type.match (sqlt))
            {
              r.to = t.type.replace (sqlt, t.to);
              r.from = t.type.replace (sqlt, t.from);
              sqlt = t.type.replace (sqlt, t.as);
              break;
            }
          }
        }

        sql_lexer l (sqlt);

        // While most type names use single identifier, there are
        // a couple of exceptions to this rule:
        //
        // BIT VARYING               (VARBIT)
        // CHARACTER VARYING         (VARRCHAR)
        // DOUBLE PRECISION          (DOUBLE)
        // TIME WITH TIME ZONE       (not currently supported)
        // TIMESTAMP WITH TIME ZONE  (not currently supported)
        //

        enum state
        {
          parse_prefix,
          parse_name,
          parse_range,
          parse_suffix,
          parse_done
        };

        state s (parse_prefix);
        string prefix;
        bool flt (false);

        for (sql_token t (l.next ());
             s != parse_done && t.type () != sql_token::t_eos;
             t = l.next ())
        {
          sql_token::token_type tt (t.type ());

          switch (s)
          {
          case parse_prefix:
            {
              if (tt == sql_token::t_identifier)
              {
                string const& id (context::upcase (t.identifier ()));

                if (id == "BIT" ||
                    id == "CHARACTER" ||
                    id == "DOUBLE")
                {
                  prefix = id;
                  s = parse_name;
                  continue;
                }
              }

              s = parse_name;
            }
            // Fall through.
          case parse_name:
            {
              if (tt == sql_token::t_identifier)
              {
                bool match (true);
                string const& id (context::upcase (t.identifier ()));

                //
                // Numeric types.
                //
                if (id == "BOOL" || id == "BOOLEAN")
                {
                  r.type = sql_type::BOOLEAN;
                }
                else if (id == "SMALLINT" || id == "INT2")
                {
                  r.type = sql_type::SMALLINT;
                }
                else if (id == "INT" ||
                         id == "INTEGER" ||
                         id == "INT4")
                {
                  r.type = sql_type::INTEGER;
                }
                else if (id == "BIGINT")
                {
                  r.type = sql_type::BIGINT;
                }
                else if (id == "REAL" || id == "FLOAT4")
                {
                  r.type = sql_type::REAL;
                }
                else if ((id == "PRECISION" && prefix == "DOUBLE") ||
                         id == "FLOAT8")
                {
                  r.type = sql_type::DOUBLE;
                }
                else if (id == "FLOAT")
                {
                  // Assign a type only once we know the precision of the
                  // float.
                  //
                  flt = true;
                }
                else if (id == "NUMERIC" || id == "DECIMAL")
                {
                  r.type = sql_type::NUMERIC;
                }
                //
                // Date-time types.
                //
                else if (id == "DATE")
                {
                  r.type = sql_type::DATE;
                }
                else if (id == "TIME")
                {
                  r.type = sql_type::TIME;
                }
                else if (id == "TIMETZ")
                {
                  return error (ct, "PostgreSQL time zones are not currently "
                                "supported");
                }
                else if (id == "TIMESTAMP")
                {
                  r.type = sql_type::TIMESTAMP;
                }
                else if (id == "TIMESTAMPTZ")
                {
                  return error (ct, "PostgreSQL time zones are not currently "
                                "supported");
                }
                //
                // String and binary types.
                //
                else if (id == "CHAR")
                {
                  r.type = sql_type::CHAR;
                }
                else if (id == "VARCHAR")
                {
                  r.type = sql_type::VARCHAR;
                }
                else if (id == "TEXT")
                {
                  r.type = sql_type::TEXT;
                }
                else if (id == "VARYING")
                {
                  if (prefix == "BIT")
                    r.type = sql_type::VARBIT;
                  else if (prefix == "CHARACTER")
                    r.type = sql_type::VARCHAR;
                }
                else if (id == "BYTEA")
                {
                  r.type = sql_type::BYTEA;
                }
                else if (id == "VARBIT")
                {
                  r.type = sql_type::VARBIT;
                }
                //
                // Other types.
                //
                else if (id == "UUID")
                {
                  r.type = sql_type::UUID;
                }
                else
                  match = false;

                if (match)
                {
                  s = parse_range;
                  continue;
                }
              }

              // Some prefixes can also be type names if not followed
              // by the actual type name.
              //
              if (!prefix.empty ())
              {
                if (prefix == "BIT")
                {
                  r.type = sql_type::BIT;
                }
                else if (prefix == "CHARACTER")
                {
                  r.type = sql_type::CHAR;
                }
              }

              if (r.type == sql_type::invalid)
              {
                return error (
                  ct,
                  tt == sql_token::t_identifier
                  ? "unknown PostgreSQL type '" + t.identifier () + "'"
                  : "expected PostgreSQL type name");
              }

              s = parse_range;
            }
            // Fall through.
          case parse_range:
            {
              if (t.punctuation () == sql_token::p_lparen)
              {
                t = l.next ();

                if (t.type () != sql_token::t_int_lit)
                {
                  return error (ct, "integer range expected in PostgreSQL "
                                "type declaration");
                }

                unsigned int v;
                istringstream is (t.literal ());

                if (!(is >> v && is.eof ()))
                {
                  return error (ct, "invalid range value '" + t.literal () +
                                "' in PostgreSQL type declaration");
                }

                r.range = true;
                r.range_value = v;

                t = l.next ();

                if (t.punctuation () == sql_token::p_comma)
                {
                  // We have the second range value. Skip it.
                  //
                  l.next ();
                  t = l.next ();
                }

                if (t.punctuation () != sql_token::p_rparen)
                {
                  return error (ct, "expected ')' in PostgreSQL type "
                                "declaration");
                }

                s = parse_suffix;
                continue;
              }

              s = parse_suffix;
            }
            // Fall through.
          case parse_suffix:
            {
              if (r.type == sql_type::TIME || r.type == sql_type::TIMESTAMP)
              {
                string const& id1 (context::upcase (t.identifier ()));

                if (id1 == "WITH")
                {
                  t = l.next ();
                  tt = t.type ();

                  if (tt == sql_token::t_identifier)
                  {
                    string const& id2 (context::upcase (t.identifier ()));

                    if (id2 == "TIME")
                    {
                      t = l.next ();
                      tt = t.type ();

                      if (tt == sql_token::t_identifier)
                      {
                        string const& id3 (context::upcase (t.identifier ()));

                        if (id3 == "ZONE")
                        {
                          // This code shall not fall through.
                          //
                          return error (ct, "PostgreSQL time zones are not "
                                        "currently supported");
                        }
                      }
                    }
                  }
                }
              }

              return error (
                ct,
                tt == sql_token::t_identifier
                ? "unknown PostgreSQL type '" + t.identifier () + "'"
                : "unknown PostgreSQL type");
            }
          case parse_done:
            {
              assert (false);
              break;
            }
          }
        }

        if (s == parse_name && !prefix.empty ())
        {
          // Some prefixes can also be type names if not followed
          // by the actual type name.
          //
          if (prefix == "BIT")
          {
            r.type = sql_type::BIT;
          }
          else if (prefix == "CHARACTER")
          {
            r.type = sql_type::CHAR;
          }
        }

        if (flt)
        {
          r.type = r.range && r.range_value < 25 ?
            sql_type::REAL :
            sql_type::DOUBLE;
        }

        if (r.type == sql_type::invalid)
          return error (ct, "incomplete PostgreSQL type declaration");

        // If range is omitted for CHAR or BIT types, it defaults to 1.
        //
        if ((r.type == sql_type::CHAR || r.type == sql_type::BIT) && !r.range)
        {
          r.range = true;
          r.range_value = 1;
        }

        return r;
      }
      catch (sql_lexer::invalid_input const& e)
      {
        return error (ct, "invalid PostgreSQL type declaration: " + e.message);
      }
    }
  }
}
