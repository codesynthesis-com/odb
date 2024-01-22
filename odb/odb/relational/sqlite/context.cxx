// file      : odb/relational/sqlite/context.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <vector>
#include <cassert>
#include <sstream>

#include <odb/sql-token.hxx>
#include <odb/sql-lexer.hxx>

#include <odb/relational/sqlite/context.hxx>
#include <odb/relational/sqlite/common.hxx>

using namespace std;

namespace relational
{
  namespace sqlite
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
        {"bool", "INTEGER", 0, false},

        {"char", "TEXT", 0, false},
        {"wchar_t", "TEXT", 0, false},
        {"signed char", "INTEGER", 0, false},
        {"unsigned char", "INTEGER", 0, false},

        {"short int", "INTEGER", 0, false},
        {"short unsigned int", "INTEGER", 0, false},

        {"int", "INTEGER", 0, false},
        {"unsigned int", "INTEGER", 0, false},

        {"long int", "INTEGER", 0, false},
        {"long unsigned int", "INTEGER", 0, false},

        {"long long int", "INTEGER", 0, false},
        {"long long unsigned int", "INTEGER", 0, false},

        // SQLite stores NaN as NULL.
        //
        {"float", "REAL", 0, true},
        {"double", "REAL", 0, true},

        {"::std::string", "TEXT", 0, false},
        {"::std::wstring", "TEXT", 0, false}
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
      global_index = true;
      global_fkey = false;
      data_->bind_vector_ = "sqlite::bind*";
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

    string const& context::
    convert_expr (string const& sqlt, semantics::data_member& m, bool to)
    {
      sql_type const& t (parse_sql_type (sqlt, m));
      return to ? t.to : t.from;
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

          if (section_ == 0 && c.count ("sqlite-grow"))
            r_ = c.get<bool> ("sqlite-grow");
          else
          {
            // r_ should be false.
            //
            inherits (c);

            if (!r_)
              names (c);

            if (section_ == 0)
              c.set ("sqlite-grow", r_);
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
        traverse_string (member_info&)
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
      if (section == 0 && c.count ("sqlite-grow"))
        return c.get<bool> ("sqlite-grow");

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
        if (bt.is_a<semantics::fund_char> () ||
            bt.is_a<semantics::fund_wchar> ())
        {
          if (a->size () != 0)
            r = "TEXT";
        }
      }

      return r;
    }

    //
    // SQL type parsing.
    //

    namespace
    {
      struct sql_parser
      {
        typedef context::invalid_sql_type invalid_sql_type;

        sql_parser (custom_db_types const* ct): ct_ (ct) {}

        sql_type
        parse (string sql)
        {
          sql_type r;

          // First run the type through the custom mapping, if requested.
          //
          if (ct_ != 0)
          {
            for (custom_db_types::const_iterator i (ct_->begin ());
                 i != ct_->end (); ++i)
            {
              custom_db_type const& t (*i);

              if (t.type.match (sql))
              {
                r.to = t.type.replace (sql, t.to);
                r.from = t.type.replace (sql, t.from);
                sql = t.type.replace (sql, t.as);
                break;
              }
            }
          }

          // Parse the type into a sequence of identifiers.
          //
          try
          {
            l_.lex (sql);

            for (sql_token t (l_.next ()); t.type () != sql_token::t_eos;)
            {
              sql_token::token_type tt (t.type ());

              if (tt == sql_token::t_identifier)
              {
                ids_.push_back (context::upcase (t.identifier ()));
                t = l_.next ();

                if (t.punctuation () == sql_token::p_lparen)
                {
                  if (!parse_range ())
                    return error (m_);

                  t = l_.next ();
                }
              }
              else
                return error ("expected SQLite type name instead of '" +
                              t.string () + "'");
            }
          }
          catch (sql_lexer::invalid_input const& e)
          {
            return error ("invalid SQLite type declaration: " + e.message);
          }

          if (ids_.empty ())
            return error ("expected SQLite type name");

          // First check our own types.
          //
          if (ids_.size () == 2 && ids_[0] == "TEXT" && ids_[1] == "STREAM")
          {
            r.type = sql_type::TEXT;
            r.stream = true;
          }
          if (ids_.size () == 2 && ids_[0] == "BLOB" && ids_[1] == "STREAM")
          {
            r.type = sql_type::BLOB;
            r.stream = true;
          }
          //
          // Apply the first four rules of the SQLite type to affinity
          // conversion algorithm.
          //
          else if (find ("INT"))
            r.type = sql_type::INTEGER;
          else if (find ("TEXT") || find ("CHAR") || find ("CLOB"))
            r.type = sql_type::TEXT;
          else if (find ("BLOB"))
            r.type = sql_type::BLOB;
          else if (find ("REAL") || find ("FLOA") || find ("DOUB"))
            r.type = sql_type::REAL;
          else
          {
            // Instead of the fifth rule which maps everything else
            // to NUMERICAL (which we don't have), map some commonly
            // used type names to one of the above types.
            //
            string const& id (ids_[0]);

            if (id == "NUMERIC")
              r.type = sql_type::REAL;
            else if (id == "DECIMAL")
              r.type = sql_type::TEXT;
            else if (id == "BOOLEAN" || id == "BOOL")
              r.type = sql_type::INTEGER;
            else if (id == "DATE" || id == "TIME" || id == "DATETIME")
              r.type = sql_type::TEXT;
            else
              return error ("unknown SQLite type '" + id + "'");
          }

          return r;
        }

        bool
        parse_range ()
        {
          // Skip tokens until we get the closing paren.
          //
          for (sql_token t (l_.next ());; t = l_.next ())
          {
            if (t.punctuation () == sql_token::p_rparen)
              break;

            if (t.type () == sql_token::t_eos)
            {
              m_ = "missing ')' in SQLite type declaration";
              return false;
            }
          }

          return true;
        }

      private:
        sql_type
        error (string const& m)
        {
          if (ct_ == 0)
            return sql_type ();
          else
            throw invalid_sql_type (m);
        }

        bool
        find (string const& str) const
        {
          for (identifiers::const_iterator i (ids_.begin ());
               i != ids_.end (); ++i)
          {
            if (i->find (str) != string::npos)
              return true;
          }

          return false;
        }

      private:
        custom_db_types const* ct_;
        sql_lexer l_;
        string m_; // Error message.

        typedef vector<string> identifiers;
        identifiers ids_;
      };
    }

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

    sql_type context::
    parse_sql_type (string const& sqlt, custom_db_types const* ct)
    {
      sql_parser p (ct);
      return p.parse (sqlt);
    }
  }
}
