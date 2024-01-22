// file      : odb/relational/mysql/context.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>
#include <sstream>

#include <odb/sql-token.hxx>
#include <odb/sql-lexer.hxx>

#include <odb/relational/mysql/context.hxx>
#include <odb/relational/mysql/common.hxx>

using namespace std;

namespace relational
{
  namespace mysql
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
        {"bool", "TINYINT(1)", 0, false},

        {"char", "CHAR(1)", 0, false},
        {"signed char", "TINYINT", 0, false},
        {"unsigned char", "TINYINT UNSIGNED", 0, false},

        {"short int", "SMALLINT", 0, false},
        {"short unsigned int", "SMALLINT UNSIGNED", 0, false},

        {"int", "INT", 0, false},
        {"unsigned int", "INT UNSIGNED", 0, false},

        {"long int", "BIGINT", 0, false},
        {"long unsigned int", "BIGINT UNSIGNED", 0, false},

        {"long long int", "BIGINT", 0, false},
        {"long long unsigned int", "BIGINT UNSIGNED", 0, false},

        {"float", "FLOAT", 0, false},
        {"double", "DOUBLE", 0, false},

        {"::std::string", "TEXT", "VARCHAR(128)", false},

        {"::size_t", "BIGINT UNSIGNED", 0, false},
        {"::std::size_t", "BIGINT UNSIGNED", 0, false}
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
      insert_send_auto_id = true;
      delay_freeing_statement_result = false;
      need_image_clone = false;
      generate_bulk = false;
      global_index = false;
      global_fkey = true;
      data_->bind_vector_ = "MYSQL_BIND*";
      data_->truncated_vector_ = "my_bool*";

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

        // Warn if the name is greater than the 64 limit.
        //
        if (i->size () > 64)
        {
          cerr << "warning: SQL name '" << *i << "' is longer than "
               << "the MySQL name limit of 64 characters and will "
               << "be truncated" << endl;

          cerr << "info: consider shortening it using #pragma db "
               << "table/column/index or --*-regex options" << endl;
        }

        if (f)
          f = false;
        else
          r += '.';

        r += '`';
        r.append (*i, 0, 64); // Max identifier length is 64.
        r += '`';
      }

      return r;
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
          bool view (context::view (c));

          // Ignore transient bases.
          //
          if (!(context::object (c) || view || context::composite (c)))
            return;

          if (section_ == 0 && c.count ("mysql-grow"))
            r_ = c.get<bool> ("mysql-grow");
          else
          {
            // r_ should be false.
            //
            if (!view)
              inherits (c);

            if (!r_)
              names (c);

            if (section_ == 0)
              c.set ("mysql-grow", r_);
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
        traverse_decimal (member_info&)
        {
          r_ = true;
        }

        virtual void
        traverse_long_string (member_info&)
        {
          r_ = true;
        }

        virtual void
        traverse_short_string (member_info&)
        {
          r_ = true; // @@ Short string optimization disabled.
        }

        virtual void
        traverse_enum (member_info&)
        {
          r_ = true;
        }

        virtual void
        traverse_set (member_info&)
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
      if (section == 0 && c.count ("mysql-grow"))
        return c.get<bool> ("mysql-grow");

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
      has_grow_member mt (r);
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
      has_grow_member mt (r, 0, &t, ct, kp);
      mt.traverse (m, true);
      return r;
    }

    string context::
    database_type_impl (semantics::type& t,
                        semantics::names* hint,
                        bool id,
                        bool* null)
    {
      using semantics::enum_;
      using semantics::enumerator;
      using semantics::array;

      string r;

      // Enum mapping.
      //
      if (enum_* e = dynamic_cast<enum_*> (&t))
      {
        // We can only map to ENUM if the C++ enumeration is contiguous
        // and starts with 0.
        //
        enum_::enumerates_iterator i (e->enumerates_begin ()),
          end (e->enumerates_end ());

        if (i != end)
        {
          r += "ENUM(";

          for (unsigned long long j (0); i != end; ++i, ++j)
          {
            enumerator const& er (i->enumerator ());

            if (er.value () != j)
              break;

            if (j != 0)
              r += ", ";

            r += quote_string (er.name ());
          }

          if (i == end)
            r += ")";
          else
            r.clear ();
        }

        if (!r.empty ())
          return r;
      }

      r = base_context::database_type_impl (t, hint, id, null);

      if (!r.empty ())
        return r;

      // char[N] mapping.
      //
      else if (array* a = dynamic_cast<array*> (&t))
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
        // NATIONAL CHAR|VARCHAR
        // CHAR BYTE             (BINARY)
        // CHARACTER VARYING     (VARCHAR)
        // LONG VARBINARY        (MEDIUMBLOB)
        // LONG VARCHAR          (MEDIUMTEXT)
        //
        //
        enum state
        {
          parse_prefix,
          parse_name,
          parse_range,
          parse_sign,
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

                if (id == "NATIONAL" ||
                    id == "CHAR" ||
                    id == "CHARACTER" ||
                    id == "LONG")
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

                // Numeric types.
                //
                if (id == "BIT")
                {
                  r.type = sql_type::BIT;
                }
                else if (id == "TINYINT" || id == "INT1")
                {
                  r.type = sql_type::TINYINT;
                }
                else if (id == "BOOL" || id == "BOOLEAN")
                {
                  r.type = sql_type::TINYINT;
                  r.range = true;
                  r.range_value = 1;
                }
                else if (id == "SMALLINT" || id == "INT2")
                {
                  r.type = sql_type::SMALLINT;
                }
                else if (id == "MEDIUMINT" ||
                         id == "INT3" ||
                         id == "MIDDLEINT")
                {
                  r.type = sql_type::MEDIUMINT;
                }
                else if (id == "INT" || id == "INTEGER" || id == "INT4")
                {
                  r.type = sql_type::INT;
                }
                else if (id == "BIGINT" || id == "INT8")
                {
                  r.type = sql_type::BIGINT;
                }
                else if (id == "SERIAL")
                {
                  r.type = sql_type::BIGINT;
                  r.unsign = true;
                }
                else if (id == "FLOAT")
                {
                  // Assign a type only once we know the precision of the
                  // float; it can be either 4 or 8 byte.
                  //
                  flt = true;
                }
                else if (id == "FLOAT4")
                {
                  r.type = sql_type::FLOAT;
                }
                else if (id == "DOUBLE" || id == "FLOAT8")
                {
                  r.type = sql_type::DOUBLE;
                }
                else if (id == "DECIMAL" ||
                         id == "DEC" ||
                         id == "NUMERIC" ||
                         id == "FIXED")
                {
                  r.type = sql_type::DECIMAL;
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
                else if (id == "DATETIME")
                {
                  r.type = sql_type::DATETIME;
                }
                else if (id == "TIMESTAMP")
                {
                  r.type = sql_type::TIMESTAMP;
                }
                else if (id == "YEAR")
                {
                  r.type = sql_type::YEAR;
                }
                //
                // String and binary types.
                //
                else if (id == "NCHAR")
                {
                  r.type = sql_type::CHAR;
                }
                else if (id == "VARCHAR")
                {
                  r.type = prefix == "LONG"
                    ? sql_type::MEDIUMTEXT
                    : sql_type::VARCHAR;
                }
                else if (id == "NVARCHAR")
                {
                  r.type = sql_type::VARCHAR;
                }
                else if (id == "VARYING" && prefix == "CHARACTER")
                {
                  r.type = sql_type::VARCHAR;
                }
                else if (id == "BINARY")
                {
                  r.type = sql_type::BINARY;
                }
                else if (id == "BYTE" && prefix == "CHAR")
                {
                  r.type = sql_type::BINARY;
                }
                else if (id == "VARBINARY")
                {
                  r.type = prefix == "LONG"
                    ? sql_type::MEDIUMBLOB
                    : sql_type::VARBINARY;
                }
                else if (id == "TINYBLOB")
                {
                  r.type = sql_type::TINYBLOB;
                }
                else if (id == "TINYTEXT")
                {
                  r.type = sql_type::TINYTEXT;
                }
                else if (id == "BLOB")
                {
                  r.type = sql_type::BLOB;
                }
                else if (id == "TEXT")
                {
                  r.type = sql_type::TEXT;
                }
                else if (id == "MEDIUMBLOB")
                {
                  r.type = sql_type::MEDIUMBLOB;
                }
                else if (id == "MEDIUMTEXT")
                {
                  r.type = sql_type::MEDIUMTEXT;
                }
                else if (id == "LONGBLOB")
                {
                  r.type = sql_type::LONGBLOB;
                }
                else if (id == "LONGTEXT")
                {
                  r.type = sql_type::LONGTEXT;
                }
                else if (id == "ENUM")
                {
                  r.type = sql_type::ENUM;
                }
                else if (id == "SET")
                {
                  r.type = sql_type::SET;
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
                if (prefix == "CHAR" || prefix == "CHARACTER")
                {
                  r.type = sql_type::CHAR;
                }
                else if (prefix == "LONG")
                {
                  r.type = sql_type::MEDIUMTEXT;
                }
              }

              if (r.type == sql_type::invalid)
              {
                if (tt == sql_token::t_identifier)
                {
                  return error (ct, "unknown MySQL type '" + t.identifier () +
                                "'");
                }
                else
                  return error (ct, "expected MySQL type name");
              }

              s = parse_range;
            }
            // Fall through.
          case parse_range:
            {
              if (t.punctuation () == sql_token::p_lparen)
              {
                t = l.next ();

                // ENUM and SET have a list of members instead of the range.
                //
                if (r.type == sql_type::ENUM || r.type == sql_type::SET)
                {
                  while (true)
                  {
                    if (t.type () != sql_token::t_string_lit)
                    {
                      return error (ct, "string literal expected in MySQL "
                                    "ENUM or SET declaration");
                    }

                    if (r.type == sql_type::ENUM)
                      r.enumerators.push_back (t.literal ());

                    t = l.next ();

                    if (t.punctuation () == sql_token::p_rparen)
                      break;
                    else if (t.punctuation () != sql_token::p_comma)
                    {
                      return error (ct, "comma expected in MySQL ENUM or "
                                    "SET declaration");
                    }

                    t = l.next ();
                  }
                }
                else
                {
                  if (t.type () != sql_token::t_int_lit)
                  {
                    return error (ct, "integer range expected in MySQL type "
                                  "declaration");
                  }

                  unsigned int v;
                  istringstream is (t.literal ());

                  if (!(is >> v && is.eof ()))
                  {
                    return error (ct, "invalid range value '" + t.literal () +
                                  "' in MySQL type declaration");
                  }

                  r.range = true;
                  r.range_value = v;

                  t = l.next ();

                  if (t.punctuation () == sql_token::p_comma)
                  {
                    // We have the second range value. Skip it.
                    //
                    // In FLOAT the two-value range means something
                    // completely different than the single-value.
                    // Pretend we don't have the range in the former
                    // case.
                    //
                    if (flt)
                      r.range = false;

                    l.next ();
                    t = l.next ();
                  }
                }

                if (t.punctuation () != sql_token::p_rparen)
                {
                  return error (ct, "expected ')' in MySQL type declaration");
                }

                s = parse_sign;
                continue;
              }

              s = parse_sign;
            }
            // Fall through.
          case parse_sign:
            {
              if (tt == sql_token::t_identifier &&
                  context::upcase (t.identifier ()) == "UNSIGNED")
              {
                r.unsign = true;
              }

              s = parse_done;
              break;
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
          if (prefix == "CHAR" || prefix == "CHARACTER")
          {
            r.type = sql_type::CHAR;
          }
          else if (prefix == "LONG")
          {
            r.type = sql_type::MEDIUMTEXT;
          }
        }

        if (flt)
        {
          r.type = !r.range || r.range_value < 24
            ? sql_type::FLOAT
            : sql_type::DOUBLE;
        }

        if (r.type == sql_type::invalid)
          return error (ct, "incomplete MySQL type declaration");

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
        return error (ct, "invalid MySQL type declaration: " + e.message);
      }
    }
  }
}
