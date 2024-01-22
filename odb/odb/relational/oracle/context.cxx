// file      : odb/relational/oracle/context.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>
#include <sstream>

#include <odb/sql-token.hxx>
#include <odb/sql-lexer.hxx>

#include <odb/relational/oracle/context.hxx>

using namespace std;

namespace relational
{
  namespace oracle
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
        {"bool", "NUMBER(1)", 0, false},

        {"char", "CHAR(1)", 0, false},
        {"signed char", "NUMBER(3)", 0, false},
        {"unsigned char", "NUMBER(3)", 0, false},

        {"short int", "NUMBER(5)", 0, false},
        {"short unsigned int", "NUMBER(5)", 0, false},

        {"int", "NUMBER(10)", 0, false},
        {"unsigned int", "NUMBER(10)", 0, false},

        {"long int", "NUMBER(19)", 0, false},
        {"long unsigned int", "NUMBER(20)", 0, false},

        {"long long int", "NUMBER(19)", 0, false},
        {"long long unsigned int", "NUMBER(20)", 0, false},

        {"float", "BINARY_FLOAT", 0, false},
        {"double", "BINARY_DOUBLE", 0, false},

        // Oracle treats empty VARCHAR2 (and NVARCHAR2) strings as NULL.
        //
        {"::std::string", "VARCHAR2(512)", 0, true},

        {"::size_t", "NUMBER(20)", 0, false},
        {"::std::size_t", "NUMBER(20)", 0, false}
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

      generate_grow = false;
      need_alias_as = false;
      insert_send_auto_id = false;
      delay_freeing_statement_result = false;
      need_image_clone = true;
      generate_bulk = true;
      global_index = true;
      global_fkey = true;
      data_->bind_vector_ = "oracle::bind*";

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

        if (f)
          f = false;
        else
          r += '.';

        r += '"';
        r.append (*i, 0, 30); // Max identifier length is 30.
        r += '"';
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
            r = "CHAR";
          else
          {
            r = "VARCHAR2";
            n--;
          }

          // Oracle VARCHAR2 limit is 4000 bytes. Since there are no good
          // alternatives (CLOB?), let the user specify the mapping.
          //
          if (n > 4000)
            return "";

          // Allow empty VARCHAR2 values.
          //
          if (null != 0 && r == "VARCHAR2")
            *null = true;

          ostringstream ostr;
          ostr << n;
          r += '(';
          r += ostr.str ();
          r += ')';
        }
      }

      return r;
    }

    bool context::
    unsigned_integer (semantics::type& t)
    {
      semantics::type* wt (wrapper (t));
      const string& s ((wt == 0 ? t : utype (*wt)).name ());

      return s == "bool" ||
        s == "unsigned char" ||
        s == "short unsigned int" ||
        s == "unsigned int" ||
        s == "long unsigned int" ||
        s == "long long unsigned int";
    }

    qname context::
    sequence_name (qname const& table)
    {
      string n;

      if (options.sequence_suffix ().count (db) != 0)
        n = table.uname () + options.sequence_suffix ()[db];
      else
        n = compose_name (table.uname (), "seq");

      n = transform_name (n, sql_name_sequence);

      qname r (table.qualifier ());
      r.append (n);
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
        // CHARACTER VARYING          (VARCHAR2)
        // CHAR VARYING               (VARCHAR2)
        // NATIONAL CHARACTER         (NCHAR)
        // NATIONAL CHAR              (NCHAR)
        // NCHAR VARYING              (NVARCHAR2)
        // NATIONAL CHARACTER VARYING (NVARCHAR2)
        // NATIONAL CHAR VARYING      (NVARCHAR2)
        // NCHAR VARYING              (NVARCHAR2)
        // DOUBLE PRECISION           (FLOAT(126))
        // INTERVAL YEAR TO MONTH
        // INTERVAL DAY TO SECOND
        //
        enum state
        {
          parse_identifier,
          parse_prec,
          parse_done
        };

        state s (parse_identifier);
        string prefix;
        sql_token t (l.next ());

        while (t.type () != sql_token::t_eos)
        {
          sql_token::token_type tt (t.type ());

          switch (s)
          {
          case parse_identifier:
            {
              if (tt == sql_token::t_identifier)
              {
                string const& id (context::upcase (t.identifier ()));

                //
                // Numeric types.
                //
                if ((id == "NUMBER") && prefix.empty ())
                {
                  // If NUMBER has no precision/scale, then it is a floating-
                  // point number. We indicate this by having no precision.
                  //
                  r.type = sql_type::NUMBER;
                  s = parse_prec;
                }
                else if ((id == "DEC" || id == "DECIMAL" || id == "NUMERIC")
                         && prefix.empty ())
                {
                  // DEC, DECIMAL, and NUMERIC are equivalent to NUMBER in
                  // all ways except that they may not represent a floating
                  // point number. The scale defaults to zero.
                  //
                  r.type = sql_type::NUMBER;
                  s = parse_prec;
                }
                else if ((id == "INT" || id == "INTEGER" || id == "SMALLINT")
                         && prefix.empty ())
                {
                  // INT, INTEGER, and SMALLINT map to NUMBER(38). They may not
                  // have precision or scale explicitly specified.
                  //
                  r.type = sql_type::NUMBER;
                  r.prec = true;
                  r.prec_value = 38;

                  s = parse_done;
                }
                //
                // Floating point types.
                //
                else if (id == "FLOAT" && prefix.empty ())
                {
                  r.type = sql_type::FLOAT;
                  r.prec = true;
                  r.prec_value = 126;

                  s = parse_prec;
                }
                else if (id == "DOUBLE" && prefix.empty ())
                {
                  prefix = id;
                }
                else if (id == "PRECISION" && prefix == "DOUBLE")
                {
                  r.type = sql_type::FLOAT;
                  r.prec = true;
                  r.prec_value = 126;

                  s = parse_done;
                }
                else if (id == "REAL" && prefix.empty ())
                {
                  r.type = sql_type::FLOAT;
                  r.prec = true;
                  r.prec_value = 63;

                  s = parse_done;
                }
                else if (id == "BINARY_FLOAT" && prefix.empty ())
                {
                  r.type = sql_type::BINARY_FLOAT;
                  s = parse_done;
                }
                else if (id == "BINARY_DOUBLE" && prefix.empty ())
                {
                  r.type = sql_type::BINARY_DOUBLE;
                  s = parse_done;
                }
                //
                // Date-time types.
                //
                else if (id == "DATE" && prefix.empty ())
                {
                  r.type = sql_type::DATE;
                  s = parse_done;
                }
                else if (id == "TIMESTAMP" && prefix.empty ())
                {
                  prefix = id;
                }
                else if (id == "INTERVAL" && prefix.empty ())
                {
                  prefix = id;
                }
                else if (id == "YEAR" && prefix == "INTERVAL")
                {
                  prefix += " ";
                  prefix += id;

                  r.prec = true;
                  r.prec_value = 2;
                  s = parse_prec;
                }
                else if (id == "DAY" && prefix == "INTERVAL")
                {
                  prefix += " ";
                  prefix += id;

                  r.prec = true;
                  r.prec_value = 2;
                  s = parse_prec;
                }
                else if (id == "TO" &&
                         (prefix == "INTERVAL YEAR" ||
                          prefix == "INTERVAL DAY"))
                {
                  prefix += " ";
                  prefix += id;
                }
                else if (id == "MONTH" && prefix == "INTERVAL YEAR TO")
                {
                  r.type = sql_type::INTERVAL_YM;
                  s = parse_done;
                }
                else if (id == "SECOND" && prefix == "INTERVAL DAY TO")
                {
                  r.type = sql_type::INTERVAL_DS;

                  // Store seconds precision in scale since prec holds
                  // the days precision.
                  //
                  r.scale = true;
                  r.scale_value = 6;
                  s = parse_prec;
                }
                //
                // Timestamp with time zone (not supported).
                //
                else if (id == "WITH" && prefix == "TIMESTAMP")
                {
                  prefix += " ";
                  prefix += id;
                }
                else if (id == "TIME" &&
                         (prefix == "TIMESTAMP WITH" ||
                          prefix == "TIMESTAMP WITH LOCAL"))
                {
                  prefix += " ";
                  prefix += id;
                }
                else if (id == "LOCAL" && prefix == "TIMESTAMP WITH")
                {
                  prefix += " ";
                  prefix += id;
                }
                else if (id == "ZONE" &&
                         (prefix == "TIMESTAMP WITH LOCAL TIME" ||
                          prefix == "TIMESTAMP WITH TIME"))
                {
                  return error (ct, "Oracle timestamps with time zones are "
                                "not currently supported");
                }
                //
                // String and binary types.
                //
                else if (id == "CHAR")
                {
                  prefix += prefix.empty () ? "" : " ";
                  prefix += id;
                }
                else if (id == "CHARACTER")
                {
                  prefix += prefix.empty () ? "" : " ";
                  prefix += id;
                }
                else if (id == "NCHAR")
                {
                  prefix += prefix.empty () ? "" : " ";
                  prefix += id;
                }
                else if (id == "VARCHAR" || id == "VARCHAR2")
                {
                  // VARCHAR is currently mapped to VARCHAR2 in Oracle server.
                  // However, this may change in future versions.
                  //
                  r.type = sql_type::VARCHAR2;
                  r.byte_semantics = true;
                  s = parse_prec;
                }
                else if (id == "NVARCHAR2")
                {
                  r.type = sql_type::NVARCHAR2;
                  r.byte_semantics = false;
                  s = parse_prec;
                }
                else if (id == "VARYING")
                {
                  // VARYING always appears at the end of an identifier.
                  //
                  if (prefix == "CHAR" || prefix == "CHARACTER")
                  {
                    r.type = sql_type::VARCHAR2;
                    r.byte_semantics = true;
                  }
                  else if (prefix == "NCHAR" ||
                           prefix == "NATIONAL CHAR" ||
                           prefix == "NATIONAL CHARACTER")
                  {
                    r.type = sql_type::NVARCHAR2;
                    r.byte_semantics = false;
                  }

                  s = parse_prec;
                }
                else if (id == "NATIONAL" && prefix.empty ())
                {
                  prefix = id;
                }
                else if (id == "RAW" && prefix.empty ())
                {
                  r.type = sql_type::RAW;
                  s = parse_prec;
                }
                //
                // LOB types.
                //
                else if (id == "BLOB" && prefix.empty ())
                {
                  r.type = sql_type::BLOB;
                  s = parse_done;
                }
                else if (id == "CLOB" && prefix.empty ())
                {
                  r.type = sql_type::CLOB;
                  s = parse_done;
                }
                else if (id == "NCLOB" && prefix.empty ())
                {
                  r.type = sql_type::NCLOB;
                  s = parse_done;
                }
                //
                // LONG types.
                //
                else if (id == "LONG")
                  return error (ct, "Oracle LONG types are not supported");
                else
                  return error (ct, "unknown Oracle type '" +
                                t.identifier () + "'");

                t = l.next ();
                continue;
              }
              else if (!prefix.empty ())
              {
                // Some prefixes can also be type names if not followed
                // by the actual type name.
                //

                if (prefix == "CHAR" || prefix == "CHARACTER")
                {
                  r.type = sql_type::CHAR;
                  r.byte_semantics = true;
                  r.prec = true;
                  r.prec_value = 1;
                }
                else if (prefix == "NCHAR" ||
                         prefix == "NATIONAL CHAR" ||
                         prefix == "NATIONAL CHARACTER")
                {
                  r.type = sql_type::NCHAR;
                  r.byte_semantics = false;
                  r.prec = true;
                  r.prec_value = 1;
                }
                else if (prefix == "TIMESTAMP")
                {
                  r.type = sql_type::TIMESTAMP;
                  r.prec = true;
                  r.prec_value = 6;
                }
                else
                  return error (ct, "incomplete Oracle type declaration: '" +
                                prefix + "'");

                // All of the possible types handled in this block can take
                // an optional precision specifier. Set the state and fall
                // through to the parse_prec handler.
                //
                s = parse_prec;
              }
              else
              {
                assert (r.type == sql_type::invalid);
                return error (ct, "unexepected '" + t.literal () +
                              "' in Oracle type declaration");
              }
            }
            // Fall through.
          case parse_prec:
            {
              if (t.punctuation () == sql_token::p_lparen)
              {
                t = l.next ();

                if (t.type () != sql_token::t_int_lit)
                {
                  return error (ct, "integer size/precision expected in "
                                "Oracle type declaration");
                }

                // Parse the precision.
                //
                {
                  unsigned short v;
                  istringstream is (t.literal ());

                  if (!(is >> v && is.eof ()))
                  {
                    return error (ct, "invalid prec value '" + t.literal () +
                                  "' in Oracle type declaration");
                  }

                  // Store seconds precision in scale since prec holds
                  // the days precision for INTERVAL DAY TO SECOND.
                  //
                  if (r.type == sql_type::INTERVAL_DS)
                  {
                    r.scale = true;
                    r.scale_value = static_cast<short> (v);
                  }
                  else
                  {
                    r.prec = true;
                    r.prec_value = v;
                  }

                  t = l.next ();
                }

                // Parse the scale if present.
                //
                if (t.punctuation () == sql_token::p_comma)
                {
                  // Scale can only be specified for NUMBER.
                  //
                  if (r.type != sql_type::NUMBER)
                  {
                    return error (ct, "invalid scale in Oracle type "
                                  "declaration");
                  }

                  t = l.next ();

                  if (t.type () != sql_token::t_int_lit)
                  {
                    return error (ct, "integer scale expected in Oracle type "
                                  "declaration");
                  }

                  short v;
                  istringstream is (t.literal ());

                  if (!(is >> v && is.eof ()))
                  {
                    return error (ct, "invalid scale value '" + t.literal () +
                                  "' in Oracle type declaration");
                  }

                  r.scale = true;
                  r.scale_value = v;

                  t = l.next ();
                }
                else if (t.type () == sql_token::t_identifier)
                {
                  const string& id (context::upcase (t.identifier ()));

                  if (id == "CHAR")
                    r.byte_semantics = false;
                  else if (id != "BYTE")
                  {
                    return error (ct, "invalid keyword '" + t.literal () +
                                  "' in Oracle type declaration");
                  }

                  t = l.next ();
                }

                if (t.punctuation () != sql_token::p_rparen)
                {
                  return error (ct, "expected ')' in Oracle type declaration");
                }
                else
                  t = l.next ();
              }

              s = r.type == sql_type::invalid ? parse_identifier : parse_done;
              continue;
            }
          case parse_done:
            {
              return error (ct, "unexepected '" + t.literal () + "' in Oracle "
                            "type declaration");
              break;
            }
          }
        }

        // Some prefixes can also be type names if not followed by the actual
        // type name.
        //
        if (r.type == sql_type::invalid)
        {
          if (!prefix.empty ())
          {
            if (prefix == "CHAR" || prefix == "CHARACTER")
            {
              r.type = sql_type::CHAR;
              r.byte_semantics = true;
              r.prec = true;
              r.prec_value = 1;
            }
            else if (prefix == "NCHAR" ||
                     prefix == "NATIONAL CHAR" ||
                     prefix == "NATIONAL CHARACTER")
            {
              r.type = sql_type::NCHAR;
              r.byte_semantics = false;
              r.prec = true;
              r.prec_value = 1;
            }
            else if (prefix == "TIMESTAMP")
            {
              r.type = sql_type::TIMESTAMP;
              r.prec = true;
              r.prec_value = 6;
            }
            else
              return error (ct, "incomplete Oracle type declaration: '" +
                            prefix + "'");
          }
          else
            return error (ct, "invalid Oracle type declaration");
        }

        return r;
      }
      catch (sql_lexer::invalid_input const& e)
      {
        return error (ct, "invalid Oracle type declaration: " + e.message);
      }
    }
  }
}
