// file      : odb/relational/source.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_SOURCE_HXX
#define ODB_RELATIONAL_SOURCE_HXX

#include <map>
#include <set>
#include <list>
#include <vector>
#include <sstream>

#include <odb/diagnostics.hxx>

#include <odb/relational/context.hxx>
#include <odb/relational/common.hxx>
#include <odb/relational/schema.hxx>

namespace relational
{
  namespace source
  {
    // Column literal in a statement (e.g., in select-list, etc).
    //
    struct statement_column
    {
      statement_column (): member (0) {}
      statement_column (std::string const& tbl,
                        std::string const& col,
                        std::string const& t,
                        semantics::data_member& m,
                        std::string const& kp = "")
          : table (tbl), column (col), type (t), member (&m), key_prefix (kp)
      {
      }

      std::string table;              // Schema-qualifed and quoted table name.
      std::string column;             // Table-qualifed and quoted column expr.
      std::string type;               // Column SQL type.
      semantics::data_member* member;
      std::string key_prefix;
    };

    typedef std::list<statement_column> statement_columns;

    // Query parameter generator. A new instance is created for each
    // query, so the customized version can have a counter to implement,
    // for example, numbered parameters (e.g., $1, $2, etc). The auto_id()
    // function is called instead of next() for the automatically-assigned
    // object id member when generating the persist statement. If empty
    // string is returned, then parameter is ignored.
    //
    struct query_parameters: virtual context
    {
      typedef query_parameters base;

      query_parameters (qname const& table): table_ (table) {}

      virtual string
      next ()
      {
        return "?";
      }

      virtual string
      auto_id ()
      {
        return next ();
      }

    protected:
      qname table_;
    };

    struct object_columns: object_columns_base, virtual context
    {
      typedef object_columns base;

      object_columns (statement_kind sk,
                      statement_columns& sc,
                      query_parameters* param = 0,
                      object_section* section = 0)
          : object_columns_base (true, true, section),
            sk_ (sk), ro_ (true), sc_ (sc), param_ (param), depth_ (1)
      {
      }

      object_columns (statement_kind sk,
                      bool ignore_ro,
                      statement_columns& sc,
                      query_parameters* param)
          : object_columns_base (true, true, 0),
            sk_ (sk), ro_ (ignore_ro), sc_ (sc), param_ (param), depth_ (1)
      {
      }

      object_columns (std::string const& table_qname,
                      statement_kind sk,
                      statement_columns& sc,
                      size_t depth = 1,
                      object_section* section = 0)
          : object_columns_base (true, true, section),
            sk_ (sk),
            ro_ (true),
            sc_ (sc),
            param_ (0),
            table_name_ (table_qname),
            depth_ (depth)
      {
      }

      virtual bool
      section_test (data_member_path const& mp)
      {
        object_section& s (section (mp));

        // Include eager loaded members into the main section for
        // SELECT statements. Also include optimistic version into
        // section's SELECT and UPDATE statements.
        //
        return section_ == 0 ||
          *section_ == s ||
          (sk_ == statement_select &&
           *section_ == main_section &&
           !s.separate_load ()) ||
          (version (mp) &&
           (sk_ == statement_update || sk_ == statement_select));
      }

      virtual void
      traverse_object (semantics::class_& c)
      {
        // If we are generating a select statement and this is a derived
        // type in a polymorphic hierarchy, then we need to include base
        // columns, but do it in reverse order as well as switch the table
        // name (base columns come from different tables).
        //
        semantics::class_* poly_root (polymorphic (c));
        if (poly_root != 0 && poly_root != &c)
        {
          names (c);

          if (sk_ == statement_select && --depth_ != 0)
          {
            table_name_ = table_qname (polymorphic_base (c));
            inherits (c);
          }
        }
        else
          object_columns_base::traverse_object (c);
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_& c)
      {
        // Ignore polymorphic id references for select statements.
        //
        if (sk_ == statement_select && m.count ("polymorphic-ref"))
          return;

        semantics::data_member* im (inverse (m, key_prefix_));

        // Ignore certain columns depending on what kind of statement we are
        // generating. Columns corresponding to the inverse members are
        // only present in the select statements.
        //
        if (im != 0 && sk_ != statement_select)
          return;

        // Inverse object pointers come from a joined table.
        //
        if (im != 0)
        {
          bool poly (polymorphic (c) != 0);

          // In a polymorphic hierarchy the inverse member can be in
          // the base class, in which case we should use that table.
          //
          semantics::class_& imc (
            poly ? dynamic_cast<semantics::class_&> (im->scope ()) : c);

          semantics::data_member& id (*id_member (imc));
          semantics::type& idt (utype (id));

          if (container (*im))
          {
            // This container is a direct member of the class so the table
            // prefix is just the class table name. We don't assign join
            // aliases for container tables so use the actual table name.
            // Note that the if(!table_name_.empty ()) test may look wrong
            // at first but it is not; if table_name_ is empty then we are
            // generating a container table where we don't qualify columns
            // with tables.
            //
            string table;

            if (!table_name_.empty ())
              table = table_qname (*im, table_prefix (imc));

            instance<object_columns> oc (table, sk_, sc_);
            oc->traverse (*im, idt, "id", "object_id", &imc);
          }
          else
          {
            // Use the join alias instead of the actual table name unless we
            // are handling a container. Generally, we want the join alias
            // to be based on the column name. This is straightforward for
            // single-column references. In case of a composite id, we will
            // need to use the column prefix which is based on the data
            // member name, unless overridden by the user. In the latter
            // case the prefix can be empty, in which case we will just
            // fall back on the member's public name. Note that the
            // if(!table_name_.empty ()) test may look wrong at first but
            // it is not; if table_name_ is empty then we are generating a
            // container table where we don't qualify columns with tables.
            //
            string alias;

            if (!table_name_.empty ())
            {
              string n;

              if (composite_wrapper (idt))
              {
                n = column_prefix (m, key_prefix_, default_name_).prefix;

                if (n.empty ())
                  n = public_name_db (m);
                else if (n[n.size () - 1] == '_')
                  n.resize (n.size () - 1); // Remove trailing underscore.
              }
              else
              {
                bool dummy;
                n = column_name (m, key_prefix_, default_name_, dummy);
              }

              alias = compose_name (column_prefix_.prefix, n);

              if (poly)
              {
                qname const& table (table_name (imc));
                alias = quote_id (alias + "_" + table.uname ());
              }
              else
                alias = quote_id (alias);
            }

            instance<object_columns> oc (alias, sk_, sc_);
            oc->traverse (id);
          }
        }
        else
          object_columns_base::traverse_pointer (m, c);
      }

      virtual bool
      traverse_column (semantics::data_member& m, string const& name, bool)
      {
        // Ignore certain columns depending on what kind statement we are
        // generating. Id and readonly columns are not present in the update
        // statements.
        //
        if ((id () || readonly (member_path_, member_scope_)) &&
            sk_ == statement_update && ro_)
          return false;

        return column (m, table_name_, quote_id (name));
      }

      virtual bool
      column (semantics::data_member& m,
              string const& table,
              string const& column)
      {
        string r;

        if (!table.empty ())
        {
          r += table; // Already quoted.
          r += '.';
        }

        r += column; // Already quoted.

        string const& sqlt (column_type ());

        // Version column (optimistic concurrency) requires special
        // handling in the UPDATE statement.
        //
        //
        if (sk_ == statement_update && version (m))
        {
          r += "=" + r + "+1";
        }
        else if (param_ != 0)
        {
          r += '=';
          r += convert_to (param_->next (), sqlt, m);
        }
        else if (sk_ == statement_select)
          r = convert_from (r, sqlt, m);

        sc_.push_back (statement_column (table, r, sqlt, m, key_prefix_));
        return true;
      }

    protected:
      statement_kind sk_;
      bool ro_;
      statement_columns& sc_;
      query_parameters* param_;
      string table_name_;
      size_t depth_;
    };

    struct view_columns: object_columns_base, virtual context
    {
      typedef view_columns base;

      view_columns (statement_columns& sc): sc_ (sc), in_composite_ (false) {}

      virtual void
      traverse_composite (semantics::data_member* pm, semantics::class_& c)
      {
        if (in_composite_)
        {
          object_columns_base::traverse_composite (pm, c);
          return;
        }

        // Override the column prerix.
        //
        semantics::data_member& m (*pm);

        // If we have literal column specified, use that.
        //
        if (m.count ("column"))
        {
          table_column const& tc (m.get<table_column> ("column"));

          if (!tc.table.empty ())
            table_prefix_ = tc.table;

          column_prefix_ = object_columns_base::column_prefix (m);
        }
        // Otherwise, see if there is a column expression. For composite
        // members in a view, this should be a single reference.
        //
        else if (m.count ("column-expr"))
        {
          column_expr const& e (m.get<column_expr> ("column-expr"));

          if (e.size () > 1)
          {
            cerr << m.file () << ":" << m.line () << ":" << m.column ()
                 << ": error: column expression specified for a data member "
                 << "of a composite value type" << endl;

            throw operation_failed ();
          }

          data_member_path const& mp (e.back ().member_path);

          if (mp.size () > 1)
          {
            cerr << m.file () << ":" << m.line () << ":" << m.column ()
                 << ": error: invalid data member in db pragma column"
                 << endl;

            throw operation_failed ();
          }

          table_prefix_ = e.back ().table;
          column_prefix_ = object_columns_base::column_prefix (*mp.back ());
        }
        else
        {
          cerr << m.file () << ":" << m.line () << ":" << m.column ()
               << ": error: no column prefix provided for a view data member"
               << endl;

          cerr << m.file () << ":" << m.line () << ":" << m.column ()
               << ": info: use db pragma column to specify the column prefix"
               << endl;

          throw operation_failed ();
        }

        in_composite_ = true;
        object_columns_base::traverse_composite (pm, c);
        in_composite_ = false;
      }

      virtual bool
      traverse_column (semantics::data_member& m, string const& name, bool)
      {
        string tbl;
        string col;

        // If we are inside a composite value, use the standard
        // column name machinery.
        //
        if (in_composite_)
        {
          if (!table_prefix_.empty ())
          {
            tbl = quote_id (table_prefix_);
            col += tbl;
            col += '.';
          }

          col += quote_id (name);
        }
        // If we have literal column specified, use that.
        //
        else if (m.count ("column"))
        {
          table_column const& tc (m.get<table_column> ("column"));

          if (!tc.expr)
          {
            if (!tc.table.empty ())
            {
              tbl = quote_id (tc.table);
              col += tbl;
              col += '.';
            }

            col += quote_id (tc.column);
          }
          else
            col += tc.column;
        }
        // Otherwise, see if there is a column expression.
        //
        else if (m.count ("column-expr"))
        {
          column_expr const& e (m.get<column_expr> ("column-expr"));

          for (column_expr::const_iterator i (e.begin ()); i != e.end (); ++i)
          {
            switch (i->kind)
            {
            case column_expr_part::literal:
              {
                col += i->value;
                break;
              }
            case column_expr_part::reference:
              {
                tbl = quote_id (i->table);
                col += tbl;
                col += '.';
                col += column_qname (i->member_path);
                break;
              }
            }
          }
        }
        else
        {
          cerr << m.file () << ":" << m.line () << ":" << m.column ()
               << ": error: no column name provided for a view data member"
               << endl;

          cerr << m.file () << ":" << m.line () << ":" << m.column ()
               << ": info: use db pragma column to specify the column name"
               << endl;

          throw operation_failed ();
        }

        return column (m, tbl, col);
      }

      // The column argument is a qualified and quoted column or
      // expression.
      //
      virtual bool
      column (semantics::data_member& m,
              string const& table,
              string const& column)
      {
        string const& sqlt (column_type ());
        sc_.push_back (
          statement_column (
            table, convert_from (column, sqlt, m), sqlt, m));
        return true;
      }

    protected:
      statement_columns& sc_;
      bool in_composite_;
      qname table_prefix_; // Table corresponding to column_prefix_;
    };

    struct polymorphic_object_joins: object_columns_base, virtual context
    {
      typedef polymorphic_object_joins base;

      polymorphic_object_joins (semantics::class_& obj,
                                bool query,
                                size_t depth,
                                string const& alias = "",
                                user_section* section = 0)
          : object_columns_base (true, true),
            obj_ (obj),
            query_ (query),
            depth_ (depth),
            section_ (section),
            alias_ (alias)
      {
        // Get the table and id columns.
        //
        table_ = alias_.empty ()
          ? table_qname (obj_)
          : quote_id (alias_ + "_" + table_name (obj_).uname ());

        cols_->traverse (*id_member (obj_));
      }

      virtual void
      traverse_object (semantics::class_& c)
      {
        // If section is specified, skip bases that don't add anything
        // to load.
        //
        bool skip (false), stop (false);
        if (section_ != 0)
        {
          skip = true;

          if (section_->object == &c)
          {
            user_section& s (*section_);

            if (s.total != 0 || s.optimistic ())
              skip = false;

            section_ = s.base; // Move to the next base.

            if (section_ == 0)
              stop = true; // Stop at this base if there are no more overrides.
          }
        }
        // Skip intermediates that don't add any data members.
        //
        else if (!query_)
        {
          column_count_type const& cc (column_count (c));
          if (cc.total == cc.id + cc.separate_load)
            skip = true;
        }

        if (!skip)
        {
          std::ostringstream cond;

          qname table (table_name (c));
          string alias (alias_.empty ()
                        ? quote_id (table)
                        : quote_id (alias_ + "_" + table.uname ()));

          for (object_columns_list::iterator b (cols_->begin ()), i (b);
               i != cols_->end ();
               ++i)
          {
            if (i != b)
              cond << " AND ";

            string qn (quote_id (i->name));
            cond << alias << '.' << qn << '=' << table_ << '.' << qn;
          }

          string line ("LEFT JOIN " + quote_id (table));

          if (!alias_.empty ())
            line += (need_alias_as ? " AS " : " ") + alias;

          line += " ON " + cond.str ();

          joins.push_back (line);
        }

        if (!stop && --depth_ != 0)
          inherits (c);
      }

    public:
      strings joins;

      strings::const_iterator
      begin () const {return joins.begin ();}

      strings::const_iterator
      end () const {return joins.end ();}

    private:
      semantics::class_& obj_;
      bool query_;
      size_t depth_;
      user_section* section_;
      string alias_;
      string table_;
      instance<object_columns_list> cols_;
    };

    struct object_joins: object_columns_base, virtual context
    {
      typedef object_joins base;

      //@@ context::{cur,top}_object; might have to be created every time.
      //
      object_joins (semantics::class_& scope,
                    bool query,
                    size_t depth,
                    object_section* section = 0)
          : object_columns_base (true, true, section),
            query_ (query),
            depth_ (depth),
            table_ (table_qname (scope)),
            id_ (*id_member (scope))
      {
        id_cols_->traverse (id_);
      }

      virtual bool
      section_test (data_member_path const& mp)
      {
        object_section& s (section (mp));

        // Include eager loaded members into the main section.
        //
        return section_ == 0 ||
          *section_ == s ||
          (*section_ == main_section && !s.separate_load ());
      }

      virtual void
      traverse_object (semantics::class_& c)
      {
        // If this is a derived type in a polymorphic hierarchy, then we
        // need to include base joins, but do it in reverse order as well
        // as switch the table name (base columns come from different
        // tables).
        //
        semantics::class_* poly_root (polymorphic (c));
        if (poly_root != 0 && poly_root != &c)
        {
          names (c);

          if (query_ || --depth_ != 0)
          {
            table_ = table_qname (polymorphic_base (c));
            inherits (c);
          }
        }
        else
          object_columns_base::traverse_object (c);
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_& c)
      {
        // Ignore polymorphic id references; they are joined by
        // polymorphic_object_joins in a special way.
        //
        if (m.count ("polymorphic-ref"))
          return;

        string t, a, dt, da;
        std::ostringstream cond, dcond; // @@ diversion?

        // Derive table alias for this member. Generally, we want the
        // alias to be based on the column name. This is straightforward
        // for single-column references. In case of a composite id, we
        // will need to use the column prefix which is based on the data
        // member name, unless overridden by the user. In the latter
        // case the prefix can be empty, in which case we will just
        // fall back on the member's public name.
        //
        string alias;
        {
          string n;

          if (composite_wrapper (utype (*id_member (c))))
          {
            n = column_prefix (m, key_prefix_, default_name_).prefix;

            if (n.empty ())
              n = public_name_db (m);
            else if (n[n.size () - 1] == '_')
              n.resize (n.size () - 1); // Remove trailing underscore.
          }
          else
          {
            bool dummy;
            n = column_name (m, key_prefix_, default_name_, dummy);
          }

          alias = compose_name (column_prefix_.prefix, n);
        }

        semantics::class_* poly_root (polymorphic (c));
        bool poly (poly_root != 0);

        semantics::class_* joined_obj (0);

        if (semantics::data_member* im = inverse (m, key_prefix_))
        {
          // In a polymorphic hierarchy the inverse member can be in
          // the base class, in which case we should use that table.
          //
          semantics::class_& imc (
            poly ? dynamic_cast<semantics::class_&> (im->scope ()) : c);

          if (container (*im))
          {
            // This container is a direct member of the class so the table
            // prefix is just the class table name.
            //
            t = table_qname (*im, table_prefix (imc));

            // Container's value is our id.
            //
            instance<object_columns_list> id_cols;
            id_cols->traverse (*im, utype (id_), "value", "value");

            for (object_columns_list::iterator b (id_cols->begin ()), i (b),
                   j (id_cols_->begin ()); i != id_cols->end (); ++i, ++j)
            {

              if (i != b)
                cond << " AND ";

              cond << t << '.' << quote_id (i->name) << '=' <<
                table_ << '.' << quote_id (j->name);
            }

            // Add the join for the object itself so that we are able to
            // use it in the WHERE clause.
            //
            if (query_)
            {
              // Here we can use the most derived class instead of the
              // one containing the inverse member.
              //
              qname const& table (table_name (c));

              dt = quote_id (table);
              da = quote_id (poly ? alias + "_" + table.uname () : alias);

              semantics::data_member& id (*id_member (c));

              instance<object_columns_list> oid_cols, cid_cols;
              oid_cols->traverse (id);
              cid_cols->traverse (*im, utype (id), "id", "object_id", &c);

              for (object_columns_list::iterator b (cid_cols->begin ()), i (b),
                   j (oid_cols->begin ()); i != cid_cols->end (); ++i, ++j)
              {

                if (i != b)
                  dcond << " AND ";

                dcond << da << '.' << quote_id (j->name) << '=' <<
                  t << '.' << quote_id (i->name);
              }

              joined_obj = &c;
            }
          }
          else
          {
            qname const& table (table_name (imc));

            t = quote_id (table);
            a = quote_id (poly ? alias + "_" + table.uname () : alias);

            instance<object_columns_list> id_cols;
            id_cols->traverse (*im);

            for (object_columns_list::iterator b (id_cols->begin ()), i (b),
                   j (id_cols_->begin ()); i != id_cols->end (); ++i, ++j)
            {
              if (i != b)
                cond << " AND ";

              cond << a << '.' << quote_id (i->name) << '=' <<
                table_ << '.' << quote_id (j->name);
            }

            // If we are generating query, JOIN base/derived classes so
            // that we can use their data in the WHERE clause.
            //
            if (query_)
              joined_obj = &imc;
          }
        }
        else if (query_)
        {
          // We need the join to be able to use the referenced object
          // in the WHERE clause.
          //
          qname const& table (table_name (c));

          t = quote_id (table);
          a = quote_id (poly ? alias + "_" + table.uname () : alias);

          instance<object_columns_list> oid_cols (column_prefix_);
          oid_cols->traverse (m);

          instance<object_columns_list> pid_cols;
          pid_cols->traverse (*id_member (c));

          for (object_columns_list::iterator b (pid_cols->begin ()), i (b),
                   j (oid_cols->begin ()); i != pid_cols->end (); ++i, ++j)
          {

            if (i != b)
              cond << " AND ";

            cond << a << '.' << quote_id (i->name) << '=' <<
              table_ << '.' << quote_id (j->name);
          }

          joined_obj = &c;
        }

        if (!t.empty ())
        {
          string line ("LEFT JOIN ");
          line += t;

          if (!a.empty ())
            line += (need_alias_as ? " AS " : " ") + a;

          line += " ON ";
          line += cond.str ();

          joins.push_back (line);
        }

        // Add dependent join (i.e., an object table join via the
        // container table).
        //
        if (!dt.empty ())
        {
          string line ("LEFT JOIN ");
          line += dt;

          if (!da.empty ())
            line += (need_alias_as ? " AS " : " ") + da;

          line += " ON ";
          line += dcond.str ();

          joins.push_back (line);
        }

        // If we joined the object that is part of a polymorphic type
        // hierarchy, then we may need join its bases as well as its
        // derived types. This is only done for queries.
        //
        if (joined_obj != 0 && poly)
        {
          size_t depth (polymorphic_depth (*joined_obj));

          // Join "up" (derived).
          //
          if (joined_obj != &c)
          {
            bool t (true);                            //@@ (im)perfect forward.
            size_t d (polymorphic_depth (c) - depth); //@@ (im)perfect forward.
            instance<polymorphic_object_joins> j (*joined_obj, t, d, alias);
            j->traverse (c);
            joins.insert (joins.end (), j->joins.begin (), j->joins.end ());
          }

          // Join "down" (base).
          //
          if (joined_obj != poly_root)
          {
            bool t (true);        //@@ (im)perfect forward.
            size_t d (depth - 1); //@@ (im)perfect forward.
            instance<polymorphic_object_joins> j (*joined_obj, t, d, alias);
            j->traverse (polymorphic_base (*joined_obj));
            joins.insert (joins.end (), j->joins.begin (), j->joins.end ());
          }
        }
      }

    public:
      strings joins;

      strings::const_iterator
      begin () const {return joins.begin ();}

      strings::const_iterator
      end () const {return joins.end ();}

    private:
      bool query_;
      size_t depth_;
      string table_;
      semantics::data_member& id_;
      instance<object_columns_list> id_cols_;
    };

    //
    // bind
    //

    struct bind_member: virtual member_base
    {
      typedef bind_member base;

      // NULL section means we are generating object bind().
      //
      bind_member (string const& var = string (),
                   string const& arg = string (),
                   object_section* section = 0)
          : member_base (var, 0, string (), string (), section),
            arg_override_ (arg)
      {
      }

      bind_member (string const& var,
                   string const& arg,
                   semantics::type& t,
                   string const& fq_type,
                   string const& key_prefix)
          : member_base (var, &t, fq_type, key_prefix),
            arg_override_ (arg)
      {
      }

    protected:
      string arg_override_;
    };

    template <typename T>
    struct bind_member_impl: bind_member, virtual member_base_impl<T>
    {
      typedef bind_member_impl base_impl;

      bind_member_impl (base const& x)
          : base (x)
      {
      }

      typedef typename member_base_impl<T>::member_info member_info;

      virtual bool
      pre (member_info& mi)
      {
        if (container (mi))
          return false;

        // Treat version as present in every section.
        //
        if (section_ != 0 && !version (mi.m) && *section_ != section (mi.m))
          return false;

        // Ignore polymorphic id references; they are bound in a special
        // way.
        //
        if (mi.ptr != 0 && mi.m.count ("polymorphic-ref"))
          return false;

        std::ostringstream ostr;
        ostr << "b[n]";
        b = ostr.str ();

        arg = arg_override_.empty () ? string ("i") : arg_override_;

        if (var_override_.empty ())
        {
          // Ignore inverse, separately-loaded members in the main
          // section (nothing to persist).
          //
          if (section_ == 0 && separate_load (mi.m) && inverse (mi.m))
            return false;

          semantics::class_* comp (composite (mi.t));

          os << "// " << mi.m.name () << endl
             << "//" << endl;

          // Order of these tests is important.
          //
          if (!insert_send_auto_id && id (mi.m) && auto_ (mi.m))
            os << "if (sk != statement_insert && sk != statement_update)"
               << "{";
          else if (section_ == 0 && separate_load (mi.m))
            os << "if (sk == statement_insert)"
               << "{";
          else if (inverse (mi.m, key_prefix_) || version (mi.m))
            os << "if (sk == statement_select)"
               << "{";
          // If the whole class is readonly, then we will never be
          // called with sk == statement_update.
          //
          else if (!readonly (*context::top_object))
          {
            if (id (mi.m) ||
                readonly (mi.m) ||
                (comp != 0 && readonly (*comp)) ||
                (section_ == 0 && separate_update (mi.m)))
              os << "if (sk != statement_update)"
                 << "{";
          }

          // If the member is soft- added or deleted, check the version.
          //
          unsigned long long av (added (mi.m));
          unsigned long long dv (deleted (mi.m));

          // If this is a composite member, see if it is summarily
          // added/deleted.
          //
          if (comp != 0)
          {
            unsigned long long cav (added (*comp));
            unsigned long long cdv (deleted (*comp));

            if (cav != 0 && (av == 0 || av < cav))
              av = cav;

            if (cdv != 0 && (dv == 0 || dv > cdv))
              dv = cdv;
          }

          // If the addition/deletion version is the same as the section's,
          // then we don't need the test.
          //
          if (user_section* s = dynamic_cast<user_section*> (section_))
          {
            if (av == added (*s->member))
              av = 0;

            if (dv == deleted (*s->member))
              dv = 0;
          }

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
        }

        return true;
      }

      virtual void
      post (member_info& mi)
      {
        if (var_override_.empty ())
        {
          semantics::class_* comp (composite (mi.t));

          // We need to increment the index even if we skipped this
          // member due to the schema version.
          //
          unsigned long long av (added (mi.m));
          unsigned long long dv (deleted (mi.m));

          if (comp != 0)
          {
            unsigned long long cav (added (*comp));
            unsigned long long cdv (deleted (*comp));

            if (cav != 0 && (av == 0 || av < cav))
              av = cav;

            if (cdv != 0 && (dv == 0 || dv > cdv))
              dv = cdv;
          }

          if (user_section* s = dynamic_cast<user_section*> (section_))
          {
            if (av == added (*s->member))
              av = 0;

            if (dv == deleted (*s->member))
              dv = 0;
          }

          if (av != 0 || dv != 0)
            os << "}";

          if (comp != 0)
          {
            bool ro (readonly (*comp));
            column_count_type const& cc (column_count (*comp));

            os << "n += " << cc.total << "UL";

            // select = total
            // insert = total - inverse
            // update = total - inverse - readonly
            //
            if (cc.inverse != 0 || (!ro && cc.readonly != 0))
            {
              os << " - (" << endl
                 << "sk == statement_select ? 0 : ";

              if (cc.inverse != 0)
                os << cc.inverse << "UL";

              if (!ro && cc.readonly != 0)
              {
                if (cc.inverse != 0)
                  os << " + ";

                os << "(" << endl
                   << "sk == statement_insert ? 0 : " <<
                  cc.readonly << "UL)";
              }

              os << ")";
            }

            os << ";";
          }
          else
            os << "n++;";

          bool block (false);

          // The same logic as in pre().
          //
          if (!insert_send_auto_id && id (mi.m) && auto_ (mi.m))
            block = true;
          else if (section_ == 0 && separate_load (mi.m))
            block = true;
          else if (inverse (mi.m, key_prefix_) || version (mi.m))
            block = true;
          else if (!readonly (*context::top_object))
          {
            semantics::class_* c;

            if (id (mi.m) ||
                readonly (mi.m) ||
                ((c = composite (mi.t)) && readonly (*c)) ||
                (section_ == 0 && separate_update (mi.m)))
              block = true;
          }

          if (block)
            os << "}";
          else
            os << endl;
        }
      }

      virtual void
      traverse_composite (member_info& mi)
      {
        os << "composite_value_traits< " << mi.fq_type () << ", id_" <<
          db << " >::bind (" << endl
           << "b + n, " << arg << "." << mi.var << "value, sk" <<
          (versioned (*composite (mi.t)) ? ", svm" : "") << ");";
      }

    protected:
      string b;
      string arg;
    };

    struct bind_base: traversal::class_, virtual context
    {
      typedef bind_base base;

      virtual void
      traverse (type& c)
      {
        bool obj (object (c));

        // Ignore transient bases. Not used for views.
        //
        if (!(obj || composite (c)))
          return;

        os << "// " << class_name (c) << " base" << endl
           << "//" << endl;

        // If the derived class is readonly, then we will never be
        // called with sk == statement_update.
        //
        bool ro (readonly (c));
        bool check (ro && !readonly (*context::top_object));

        if (check)
          os << "if (sk != statement_update)"
             << "{";

        if (obj)
          os << "object_traits_impl< ";
        else
          os << "composite_value_traits< ";

        os << class_fq_name (c) << ", id_" << db << " >::bind (b + n, i, sk" <<
          (versioned (c) ? ", svm" : "") << ");";

        column_count_type const& cc (column_count (c));

        os << "n += ";

        // select = total - separate_load
        // insert = total - inverse - optimistic_managed - id(auto & !sending)
        // update = total - inverse - optimistic_managed - id - readonly -
        //  separate_update
        //
        size_t select (cc.total - cc.separate_load);
        size_t insert (cc.total - cc.inverse - cc.optimistic_managed);
        size_t update (insert - cc.id - cc.readonly - cc.separate_update);

        semantics::data_member* id;
        if (!insert_send_auto_id && (id = id_member (c)) != 0 && auto_ (*id))
          insert -= cc.id;

        if (select == insert && insert == update)
          os << select << "UL;";
        else if (select != insert && insert == update)
          os << "sk == statement_select ? " << select << "UL : " <<
            insert << "UL;";
        else if (select == insert && insert != update)
          os << "sk == statement_update ? " << update << "UL : " <<
            select << "UL;";
        else
          os << "sk == statement_select ? " << select << "UL : " <<
            "sk == statement_insert ? " << insert << "UL : " <<
            update << "UL;";

        if (check)
          os << "}";
        else
          os << endl;
      }
    };

    //
    // grow
    //

    struct grow_member: virtual member_base
    {
      typedef grow_member base;

      grow_member (size_t& index,
                   string const& var = string (),
                   user_section* section = 0)
          : member_base (var, 0, string (), string (), section),
            index_ (index)
      {
      }

      grow_member (size_t& index,
                   string const& var,
                   semantics::type& t,
                   string const& fq_type,
                   string const& key_prefix)
          : member_base (var, &t, fq_type, key_prefix), index_ (index)
      {
      }

    protected:
      size_t& index_;
    };

    struct grow_base: traversal::class_, virtual context
    {
      typedef grow_base base;

      grow_base (size_t& index): index_ (index) {}

      virtual void
      traverse (type& c)
      {
        bool obj (object (c));

        // Ignore transient bases. Not used for views.
        //
        if (!(obj || composite (c)))
          return;

        os << "// " << class_name (c) << " base" << endl
           << "//" << endl;

        os << "if (";

        if (obj)
          os << "object_traits_impl< ";
        else
          os << "composite_value_traits< ";

        os << class_fq_name (c) << ", id_" << db << " >::grow (" << endl
           << "i, t + " << index_ << "UL" <<
          (versioned (c) ? ", svm" : "") << "))" << endl
           << "grew = true;"
           << endl;

        index_ += column_count (c).total;
      }

    protected:
      size_t& index_;
    };

    //
    // init image
    //

    struct init_image_member: virtual member_base
    {
      typedef init_image_member base;

      init_image_member (string const& var = string (),
                         string const& member = string (),
                         user_section* section = 0)
          : member_base (var, 0, string (), string (), section),
            member_override_ (member)
      {
      }

      init_image_member (string const& var,
                         string const& member,
                         semantics::type& t,
                         string const& fq_type,
                         string const& key_prefix)
          : member_base (var, &t, fq_type, key_prefix),
            member_override_ (member)
      {
      }

    protected:
      string member_override_;
    };

    template <typename T>
    struct init_image_member_impl: init_image_member,
                                   virtual member_base_impl<T>
    {
      typedef init_image_member_impl base_impl;

      init_image_member_impl (base const& x)
          : base (x),
            member_database_type_id_ (base::type_override_,
                                      base::fq_type_override_,
                                      base::key_prefix_)
      {
      }

      typedef typename member_base_impl<T>::member_info member_info;

      virtual void
      set_null (member_info&) = 0;

      virtual void
      check_accessor (member_info&, member_access&) {}

      virtual bool
      pre (member_info& mi)
      {
        // Ignore containers (they get their own table) and inverse
        // object pointers (they are not present in this binding).
        //
        if (container (mi) || inverse (mi.m, key_prefix_))
          return false;

        if (section_ != 0 && *section_ != section (mi.m))
          return false;

        // Ignore polymorphic id references; they are initialized in a
        // special way.
        //
        if (mi.ptr != 0 && mi.m.count ("polymorphic-ref"))
          return false;

        semantics::class_* comp (composite (mi.t));

        if (!member_override_.empty ())
        {
          member = member_override_;
          os << "{";
        }
        else
        {
          // If we are generating standard init() and this member
          // contains version, ignore it.
          //
          if (version (mi.m))
            return false;

          // If we don't send auto id in INSERT statement, ignore this
          // member altogether (we never send auto id in UPDATE).
          //
          if (!insert_send_auto_id && id (mi.m) && auto_ (mi.m))
            return false;

          os << "// " << mi.m.name () << endl
             << "//" << endl;

          // If the member is soft- added or deleted, check the version.
          //
          unsigned long long av (added (mi.m));
          unsigned long long dv (deleted (mi.m));

          // If this is a composite member, see if it is summarily
          // added/deleted.
          //
          if (comp != 0)
          {
            unsigned long long cav (added (*comp));
            unsigned long long cdv (deleted (*comp));

            if (cav != 0 && (av == 0 || av < cav))
              av = cav;

            if (cdv != 0 && (dv == 0 || dv > cdv))
              dv = cdv;
          }

          // If the addition/deletion version is the same as the section's,
          // then we don't need the test.
          //
          if (user_section* s = dynamic_cast<user_section*> (section_))
          {
            if (av == added (*s->member))
              av = 0;

            if (dv == deleted (*s->member))
              dv = 0;
          }

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

          // If the whole class is readonly, then we will never be
          // called with sk == statement_update.
          //
          if (!readonly (*context::top_object))
          {
            if (id (mi.m) ||
                readonly (mi.m) ||
                (section_ == 0 && separate_update (mi.m)) ||
                (comp != 0 && readonly (*comp))) // Can't be id.
            {
              // If we are generating section init(), then sk can only be
              // statement_update.
              //
              if (section_ == 0)
                os << "if (sk == statement_insert)";
            }
          }

          os << "{";

          if (discriminator (mi.m))
            member = "di.discriminator";
          else
          {
            // Get the member using the accessor expression.
            //
            member_access& ma (mi.m.template get<member_access> ("get"));

            // Make sure this kind of member can be accessed with this
            // kind of accessor (database-specific, e.g., streaming).
            //
            if (comp == 0)
              check_accessor (mi, ma);

            // If this is not a synthesized expression, then output
            // its location for easier error tracking.
            //
            if (!ma.synthesized)
              os << "// From " << location_string (ma.loc, true) << endl;

            // Use the original type to form the const reference. VC++
            // cannot grok the constructor syntax.
            //
            os << member_ref_type (mi.m, true, "v") << " =" << endl
               << "  " << ma.translate ("o") << ";"
               << endl;

            member = "v";
          }
        }

        // If this is a wrapped composite value, then we need to "unwrap"
        // it. If this is a NULL wrapper, then we also need to handle that.
        // For simple values this is taken care of by the value_traits
        // specializations.
        //
        if (mi.wrapper != 0 && comp != 0)
        {
          // The wrapper type, not the wrapped type.
          //
          string const& wt (mi.fq_type (false));

          // If this is a NULL wrapper and the member can be NULL, then
          // we need to handle the NULL value.
          //
          if (null (mi.m, key_prefix_) &&
              mi.wrapper->template get<bool> ("wrapper-null-handler"))
          {
            os << "if (wrapper_traits< " << wt << " >::get_null (" <<
              member << "))" << endl
               << "composite_value_traits< " << mi.fq_type () << ", id_" <<
              db << " >::set_null (" << endl
               << "i." << mi.var << "value, sk" <<
              (versioned (*comp) ? ", svm" : "") << ");"
               << "else"
               << "{";
          }

          member = "wrapper_traits< " + wt + " >::get_ref (" + member + ")";
        }

        if (discriminator (mi.m))
          os << "const info_type& di (map->find (typeid (o)));"
             << endl;

        if (mi.ptr != 0)
        {
          // When handling a pointer, mi.t is the id type of the referenced
          // object.
          //
          semantics::type& pt (member_utype (mi.m, key_prefix_));

          type = "obj_traits::id_type";

          // Handle NULL pointers and extract the id.
          //
          os << "typedef object_traits< " << class_fq_name (*mi.ptr) <<
            " > obj_traits;";

          if (weak_pointer (pt))
          {
            os << "typedef odb::pointer_traits< " << mi.ptr_fq_type () <<
              " > wptr_traits;"
               << "typedef odb::pointer_traits< wptr_traits::" <<
              "strong_pointer_type > ptr_traits;"
               << endl
               << "wptr_traits::strong_pointer_type sp (" <<
              "wptr_traits::lock (" << member << "));";

            member = "sp";
          }
          else
            os << "typedef odb::pointer_traits< " << mi.ptr_fq_type () <<
              " > ptr_traits;"
               << endl;

          os << "bool is_null (ptr_traits::null_ptr (" << member << "));"
             << "if (!is_null)"
             << "{"
             << "const " << type << "& id (" << endl;

          if (lazy_pointer (pt))
            os << "ptr_traits::object_id< ptr_traits::element_type  > (" <<
              member << ")";
          else
            os << "obj_traits::id (ptr_traits::get_ref (" << member << "))";

          os << ");"
             << endl;

          member = "id";
        }
        else if (comp != 0)
          type = mi.fq_type ();
        else
        {
          type = mi.fq_type ();

          // Indicate to the value_traits whether this column can be NULL.
          //
          os << "bool is_null (" << null (mi.m, key_prefix_) << ");";
        }

        if (comp != 0)
          traits = "composite_value_traits< " + type + ", id_" +
            db.string () + " >";
        else
        {
          db_type_id = member_database_type_id_->database_type_id (mi.m);
          traits = db.string () + "::value_traits<\n    "
            + type + ",\n    "
            + db_type_id + " >";
        }

        return true;
      }

      virtual void
      post (member_info& mi)
      {
        semantics::class_* comp (composite (mi.t));

        if (mi.ptr != 0)
        {
          os << "}"
             << "else" << endl;

          if (!null (mi.m, key_prefix_))
            os << "throw null_pointer ();";
          else if (comp != 0)
            os << traits << "::set_null (i." << mi.var << "value, sk" <<
              (versioned (*comp) ? ", svm" : "") << ");";
          else
            set_null (mi);
        }

        if (mi.wrapper != 0 && comp != 0)
        {
          if (null (mi.m, key_prefix_) &&
              mi.wrapper->template get<bool> ("wrapper-null-handler"))
            os << "}";
        }

        os << "}";

        if (member_override_.empty ())
        {
          unsigned long long av (added (mi.m));
          unsigned long long dv (deleted (mi.m));

          if (comp != 0)
          {
            unsigned long long cav (added (*comp));
            unsigned long long cdv (deleted (*comp));

            if (cav != 0 && (av == 0 || av < cav))
              av = cav;

            if (cdv != 0 && (dv == 0 || dv > cdv))
              dv = cdv;
          }

          if (user_section* s = dynamic_cast<user_section*> (section_))
          {
            if (av == added (*s->member))
              av = 0;

            if (dv == deleted (*s->member))
              dv = 0;
          }

          if (av != 0 || dv != 0)
            os << "}";
        }
      }

      virtual void
      traverse_composite (member_info& mi)
      {
        bool grow (generate_grow && context::grow (mi.m, mi.t, key_prefix_));

        if (grow)
          os << "if (";

        os << traits << "::init (" << endl
           << "i." << mi.var << "value," << endl
           << member << "," << endl
           << "sk";

        if (versioned (*composite (mi.t)))
          os << "," << endl
             << "svm";

        os << ")";

        if (grow)
          os << ")" << endl
             << "grew = true";

        os << ";";
      }

    protected:
      string type;
      string db_type_id;
      string member;
      string traits;

      instance<member_database_type_id> member_database_type_id_;
    };

    struct init_image_base: traversal::class_, virtual context
    {
      typedef init_image_base base;

      virtual void
      traverse (type& c)
      {
        bool obj (object (c));

        // Ignore transient bases. Not used for views.
        //
        if (!(obj || composite (c)))
          return;

        os << "// " << class_name (c) << " base" << endl
           << "//" << endl;

        // If the derived class is readonly, then we will never be
        // called with sk == statement_update.
        //
        bool check (readonly (c) && !readonly (*context::top_object));

        if (check)
          os << "if (sk != statement_update)"
             << "{";

        if (generate_grow)
          os << "if (";

        if (obj)
          os << "object_traits_impl< ";
        else
          os << "composite_value_traits< ";

        os << class_fq_name (c) << ", id_" << db << " >::init (i, o, sk" <<
          (versioned (c) ? ", svm" : "") << ")";

        if (generate_grow)
          os << ")" << endl
             << "grew = true";

        os << ";";

        if (check)
          os << "}";
        else
          os << endl;
      }
    };

    //
    // init value
    //

    struct init_value_member: virtual member_base
    {
      typedef init_value_member base;

      init_value_member (string const& member = string (),
                         string const& var = string (),
                         bool ignore_implicit_discriminator = true,
                         user_section* section = 0)
          : member_base (var, 0, string (), string (), section),
            member_override_ (member),
            ignore_implicit_discriminator_ (ignore_implicit_discriminator)
      {
      }

      init_value_member (string const& var,
                         string const& member,
                         semantics::type& t,
                         string const& fq_type,
                         string const& key_prefix)
          : member_base (var, &t, fq_type, key_prefix),
            member_override_ (member),
            ignore_implicit_discriminator_ (true)
      {
      }

    protected:
      string member_override_;
      bool ignore_implicit_discriminator_;
    };

    template <typename T>
    struct init_value_member_impl: init_value_member,
                                   virtual member_base_impl<T>
    {
      typedef init_value_member_impl base_impl;

      init_value_member_impl (base const& x)
          : base (x),
            member_database_type_id_ (base::type_override_,
                                      base::fq_type_override_,
                                      base::key_prefix_)
      {
      }

      typedef typename member_base_impl<T>::member_info member_info;

      virtual void
      get_null (member_info&) = 0;

      virtual void
      check_modifier (member_info&, member_access&) {}

      virtual bool
      pre (member_info& mi)
      {
        if (container (mi))
          return false;

        if (section_ != 0 && *section_ != section (mi.m))
          return false;

        // Ignore polymorphic id references; they are initialized in a
        // special way.
        //
        if (mi.ptr != 0 && mi.m.count ("polymorphic-ref"))
          return false;

        // Ignore implicit discriminators.
        //
        if (ignore_implicit_discriminator_ && discriminator (mi.m))
          return false;

        semantics::class_* comp (composite (mi.t));

        if (!member_override_.empty ())
        {
          os << "{";
          member = member_override_;
        }
        else
        {
          // Ignore separately loaded members.
          //
          if (section_ == 0 && separate_load (mi.m))
            return false;

          os << "// " << mi.m.name () << endl
             << "//" << endl;

          // If the member is soft- added or deleted, check the version.
          //
          unsigned long long av (added (mi.m));
          unsigned long long dv (deleted (mi.m));

          // If this is a composite member, see if it is summarily
          // added/deleted.
          //
          if (comp != 0)
          {
            unsigned long long cav (added (*comp));
            unsigned long long cdv (deleted (*comp));

            if (cav != 0 && (av == 0 || av < cav))
              av = cav;

            if (cdv != 0 && (dv == 0 || dv > cdv))
              dv = cdv;
          }

          // If the addition/deletion version is the same as the section's,
          // then we don't need the test.
          //
          if (user_section* s = dynamic_cast<user_section*> (section_))
          {
            if (av == added (*s->member))
              av = 0;

            if (dv == deleted (*s->member))
              dv = 0;
          }

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

          // Get the member using the accessor expression.
          //
          member_access& ma (mi.m.template get<member_access> ("set"));

          // Make sure this kind of member can be modified with this
          // kind of accessor (database-specific, e.g., streaming).
          //
          if (comp == 0)
            check_modifier (mi, ma);

          // If this is not a synthesized expression, then output
          // its location for easier error tracking.
          //
          if (!ma.synthesized)
            os << "// From " << location_string (ma.loc, true) << endl;

          // See if we are modifying via a reference or proper modifier.
          //
          if (ma.placeholder ())
            os << member_val_type (mi.m, false, "v") << ";"
               << endl;
          else
          {
            // Use the original type to form the reference. VC++ cannot
            // grok the constructor syntax.
            //
            os << member_ref_type (mi.m, false, "v") << " =" << endl
               << "  ";

            // If this member is const and we have a synthesized direct
            // access, then cast away constness. Otherwise, we assume
            // that the user-provided expression handles this.
            //
            bool cast (mi.cq && ma.direct ());
            if (cast)
              os << "const_cast< " << member_ref_type (mi.m, false) <<
                " > (" << endl;

            os << ma.translate ("o");

            if (cast)
              os << ")";

            os << ";"
               << endl;
          }

          member = "v";
        }

        // If this is a wrapped composite value, then we need to "unwrap" it.
        // If this is a NULL wrapper, then we also need to handle that. For
        // simple values this is taken care of by the value_traits
        // specializations.
        //
        if (mi.wrapper != 0 && comp != 0)
        {
          // The wrapper type, not the wrapped type.
          //
          string const& wt (mi.fq_type (false));

          // If this is a NULL wrapper and the member can be NULL, then
          // we need to handle the NULL value.
          //
          if (null (mi.m, key_prefix_) &&
              mi.wrapper->template get<bool> ("wrapper-null-handler"))
          {
            os << "if (composite_value_traits< " << mi.fq_type () <<
              ", id_" << db << " >::get_null (" << endl
               << "i." << mi.var << "value" <<
              (versioned (*comp) ? ", svm" : "") << "))" << endl
               << "wrapper_traits< " << wt << " >::set_null (" << member + ");"
               << "else" << endl;
          }

          member = "wrapper_traits< " + wt + " >::set_ref (" + member + ")";
        }

        if (mi.ptr != 0)
        {
          type = "obj_traits::id_type";

          // Handle NULL pointers and extract the id.
          //
          os << "typedef object_traits< " << class_fq_name (*mi.ptr) <<
            " > obj_traits;"
             << "typedef odb::pointer_traits< " << mi.ptr_fq_type () <<
            " > ptr_traits;"
             << endl;

          os << "if (";

          if (comp != 0)
            os << "composite_value_traits< " << type << ", id_" << db <<
              " >::get_null (" << endl
               << "i." << mi.var << "value" <<
              (versioned (*comp) ? ", svm" : "") << ")";
          else
            get_null (mi);

          os << ")" << endl;

          // Don't throw null_pointer if we can't have NULLs and the pointer
          // is NULL since this can be useful during migration. Instead, we
          // rely on the database enforcing this.
          //
          os << member << " = ptr_traits::pointer_type ();";

          os << "else"
             << "{";

          os << type << " id;";

          member = "id";
        }
        else
          type = mi.fq_type ();

        if (comp != 0)
          traits = "composite_value_traits< " + type + ", id_" +
            db.string () + " >";
        else
        {
          db_type_id = member_database_type_id_->database_type_id (mi.m);
          traits = db.string () + "::value_traits<\n    "
            + type + ",\n    "
            + db_type_id + " >";
        }

        return true;
      }

      virtual void
      post (member_info& mi)
      {
        if (mi.ptr != 0)
        {
          // Restore the member variable name.
          //
          member = member_override_.empty () ? "v" : member_override_;

          // When handling a pointer, mi.t is the id type of the referenced
          // object.
          //
          semantics::type& pt (member_utype (mi.m, key_prefix_));

          if (lazy_pointer (pt))
            os << member << " = ptr_traits::pointer_type (" << endl
               << "*static_cast<" << db << "::database*> (db), id);";
          else
          {
            os << "// If a compiler error points to the line below, then" << endl
               << "// it most likely means that a pointer used in a member" << endl
               << "// cannot be initialized from an object pointer." << endl
               << "//" << endl
               << member << " = ptr_traits::pointer_type (" << endl
               << "static_cast<" << db << "::database*> (db)->load<" << endl
               << "  obj_traits::object_type > (id));";

            // If we are loading into an eager weak pointer, make sure there
            // is someone else holding a strong pointer to it (normally a
            // session). Otherwise, the object will be loaded and immediately
            // deleted. Besides not making much sense, this also breaks the
            // delayed loading machinery which expects the object to be around
            // at least until the top-level load() returns.
            //
            if (weak_pointer (pt))
            {
              os << endl
                 << "if (odb::pointer_traits<" <<
                "ptr_traits::strong_pointer_type>::null_ptr (" << endl
                 << "ptr_traits::lock (" << member << ")))" << endl
                 << "throw session_required ();";
            }
          }

          os << "}";
        }

        // Call the modifier if we are using a proper one.
        //
        if (member_override_.empty ())
        {
          member_access& ma (mi.m.template get<member_access> ("set"));

          if (ma.placeholder ())
          {
            // If this is not a synthesized expression, then output its
            // location for easier error tracking.
            //
            if (!ma.synthesized)
              os << "// From " << location_string (ma.loc, true) << endl;

            os << ma.translate ("o", "v") << ";";
          }
        }

        os << "}";
      }

      virtual void
      traverse_composite (member_info& mi)
      {
        os << traits << "::init (" << endl
           << member << "," << endl
           << "i." << mi.var << "value," << endl
           << "db";

        if (versioned (*composite (mi.t)))
          os << "," << endl
             << "svm";

        os << ");"
           << endl;
      }

    protected:
      string type;
      string db_type_id;
      string traits;
      string member;

      instance<member_database_type_id> member_database_type_id_;
    };

    struct init_value_base: traversal::class_, virtual context
    {
      typedef init_value_base base;

      virtual void
      traverse (type& c)
      {
        bool obj (object (c));

        // Ignore transient bases. Not used for views.
        //
        if (!(obj || composite (c)))
          return;

        os << "// " << class_name (c) << " base" << endl
           << "//" << endl;

        if (obj)
          os << "object_traits_impl< ";
        else
          os << "composite_value_traits< ";

        os << class_fq_name (c) << ", id_" << db << " >::init (o, i, db" <<
          (versioned (c) ? ", svm" : "") << ");"
           << endl;
      }
    };

    // Member-specific traits types for container members.
    //
    struct container_traits: object_members_base, virtual context
    {
      typedef container_traits base;

      container_traits (semantics::class_& c)
          : object_members_base (
            true,
            object (c), // Only build table prefix for objects.
            false),
            c_ (c)
      {
        scope_ = object (c)
          ? "access::object_traits_impl< "
          : "access::composite_value_traits< ";

        scope_ += class_fq_name (c) + ", id_" + db.string () + " >";
      }

      // Unless the database system can execute several interleaving
      // statements, cache the result set.
      //
      virtual void
      cache_result (string const& statement)
      {
        os << statement << ".cache ();";
      }

      // Additional code that need to be executed following the call to
      // init_value.
      //
      virtual void
      init_value_extra ()
      {
      }

      virtual void
      process_statement_columns (statement_columns&,
                                 statement_kind,
                                 bool /*dynamic*/)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member&, semantics::class_&)
      {
        // We don't want to traverse composite id.
      }

      virtual void
      traverse_composite (semantics::data_member* m, semantics::class_& c)
      {
        if (object (c_))
          object_members_base::traverse_composite (m, c);
        else
        {
          // If we are generating traits for a composite value type, then
          // we don't want to go into its bases or it composite members.
          //
          if (m == 0 && &c == &c_)
            names (c);
        }
      }

      virtual void
      container_extra (semantics::data_member&, semantics::type&)
      {
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type& t)
      {
        using semantics::type;

        // Figure out if this member is from a base object or composite
        // value and if it's from an object, whether it is reuse-abstract.
        //
        bool base, reuse_abst;

        if (object (c_))
        {
          base = cur_object != &c_ ||
            !object (dynamic_cast<type&> (m.scope ()));
          reuse_abst = abstract (c_) && !polymorphic (c_);
        }
        else
        {
          base = false;      // We don't go into bases.
          reuse_abst = true; // Always abstract.
        }

        container_kind_type ck (container_kind (t));

        type& vt (container_vt (t));
        type* it (0);
        type* kt (0);

        semantics::data_member* im (context::inverse (m, "value"));

        bool ordered (false);
        bool inverse (im != 0);
        bool grow (false);

        switch (ck)
        {
        case ck_ordered:
          {
            if (!unordered (m))
            {
              it = &container_it (t);
              ordered = true;

              if (generate_grow)
                grow = grow || context::grow (m, *it, "index");
            }

            break;
          }
        case ck_map:
        case ck_multimap:
          {
            kt = &container_kt (t);

            if (generate_grow)
              grow = grow || context::grow (m, *kt, "key");

            break;
          }
        case ck_set:
        case ck_multiset:
          {
            break;
          }
        }

        bool smart (!inverse &&
                    (ck != ck_ordered || ordered) &&
                    container_smart (t));

        if (generate_grow)
          grow = grow || context::grow (m, vt, "value");

        bool eager_ptr (is_a (member_path_,
                              member_scope_,
                              test_eager_pointer,
                              vt,
                              "value"));
        if (!eager_ptr)
        {
          if (semantics::class_* cvt = composite_wrapper (vt))
            eager_ptr = has_a (*cvt, test_eager_pointer);
        }

        bool versioned (context::versioned (m));

        string name (flat_prefix_ + public_name (m) + "_traits");
        string scope (scope_ + "::" + name);

        os << "// " << m.name () << endl
           << "//" << endl
           << endl;

        container_extra (m, t);

        //
        // Statements.
        //
        if (!reuse_abst)
        {
          string sep (versioned ? "\n" : " ");

          semantics::type& idt (container_idt (m));

          qname table (table_name (m, table_prefix_));
          string qtable (quote_id (table));
          instance<object_columns_list> id_cols;
          instance<object_columns_list> ik_cols; // index/key columns

          if (smart)
          {
            switch (ck)
            {
            case ck_ordered:
              {
                ik_cols->traverse (m, *it, "index", "index");
                break;
              }
            case ck_map:
            case ck_multimap:
              {
                break;
              }
            case ck_set:
            case ck_multiset:
              {
                break;
              }
            }
          }

          // select_statement
          //
          os << "const char " << scope << "::" << endl
             << "select_statement[] =" << endl;

          if (inverse)
          {
            semantics::class_* c (object_pointer (vt));

            // In a polymorphic hierarchy the inverse member can be in
            // the base class, in which case we should use that class
            // for the table name, etc.
            //
            if (polymorphic (*c))
              c = &dynamic_cast<semantics::class_&> (im->scope ());

            semantics::data_member& inv_id (*id_member (*c));

            qname inv_table;                            // Other table name.
            string inv_qtable;
            instance<object_columns_list> inv_id_cols;  // Other id column.
            instance<object_columns_list> inv_fid_cols; // Other foreign id
                                                        // column (ref to us).
            statement_columns sc;

            if (container (*im))
            {
              // many(i)-to-many
              //

              // This other container is a direct member of the class so the
              // table prefix is just the class table name.
              //
              inv_table = table_name (*im, table_prefix (*c));
              inv_qtable = quote_id (inv_table);

              inv_id_cols->traverse (*im, utype (inv_id), "id", "object_id", c);
              inv_fid_cols->traverse (*im, idt, "value", "value");

              for (object_columns_list::iterator i (inv_id_cols->begin ());
                   i != inv_id_cols->end (); ++i)
              {
                // If this is a simple id, then pass the "id" key prefix. If
                // it is a composite id, then the members have no prefix.
                //
                sc.push_back (
                  statement_column (
                    inv_qtable,
                    inv_qtable + "." + quote_id (i->name),
                    i->type,
                    *i->member,
                    inv_id_cols->size () == 1 ? "id" : ""));
              }
            }
            else
            {
              // many(i)-to-one
              //
              inv_table = table_name (*c);
              inv_qtable = quote_id (inv_table);

              inv_id_cols->traverse (inv_id);
              inv_fid_cols->traverse (*im);

              for (object_columns_list::iterator i (inv_id_cols->begin ());
                   i != inv_id_cols->end (); ++i)
              {
                sc.push_back (
                  statement_column (
                    inv_qtable,
                    inv_qtable + "." + quote_id (i->name),
                    i->type,
                    *i->member));
              }
            }

            process_statement_columns (sc, statement_select, versioned);

            os << strlit ("SELECT" + sep) << endl;

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << strlit (c + (++i != e ? "," : "") + sep) << endl;
            }

            instance<query_parameters> qp (inv_table);
            os << strlit ("FROM " + inv_qtable + sep) << endl;

            string where ("WHERE ");
            for (object_columns_list::iterator b (inv_fid_cols->begin ()),
                   i (b); i != inv_fid_cols->end (); ++i)
            {
              if (i != b)
                where += " AND ";

              where += inv_qtable + "." + quote_id (i->name) + "=" +
                convert_to (qp->next (), i->type, *i->member);
            }
            os << strlit (where);
          }
          else
          {
            id_cols->traverse (m, idt, "id", "object_id");

            statement_columns sc;
            statement_kind sk (statement_select); // Imperfect forwarding.
            instance<object_columns> t (qtable, sk, sc);

            switch (ck)
            {
            case ck_ordered:
              {
                if (ordered)
                  t->traverse (m, *it, "index", "index");
                break;
              }
            case ck_map:
            case ck_multimap:
              {
                t->traverse (m, *kt, "key", "key");
                break;
              }
            case ck_set:
            case ck_multiset:
              {
                break;
              }
            }

            t->traverse (m, vt, "value", "value");

            process_statement_columns (sc, statement_select, versioned);

            os << strlit ("SELECT" + sep) << endl;

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << strlit (c + (++i != e ? "," : "") + sep) << endl;
            }

            instance<query_parameters> qp (table);
            os << strlit ("FROM " + qtable + sep) << endl;

            string where ("WHERE ");
            for (object_columns_list::iterator b (id_cols->begin ()), i (b);
                 i != id_cols->end (); ++i)
            {
              if (i != b)
                where += " AND ";

              where += qtable + "." + quote_id (i->name) + "=" +
                convert_to (qp->next (), i->type, *i->member);
            }

            if (ordered)
            {
              // Top-level column.
              //
              string const& col (
                column_qname (m, "index", "index", column_prefix ()));

              where += " ORDER BY " + qtable + "." + col;
            }

            os << strlit (where);
          }

          os << ";"
             << endl;

          // insert_statement
          //
          os << "const char " << scope << "::" << endl
             << "insert_statement[] =" << endl;

          if (inverse)
            os << strlit ("") << ";"
               << endl;
          else
          {
            statement_columns sc;
            statement_kind sk (statement_insert); // Imperfect forwarding.
            instance<object_columns> t (sk, sc);

            t->traverse (m, idt, "id", "object_id");

            switch (ck)
            {
            case ck_ordered:
              {
                if (ordered)
                  t->traverse (m, *it, "index", "index");
                break;
              }
            case ck_map:
            case ck_multimap:
              {
                t->traverse (m, *kt, "key", "key");
                break;
              }
            case ck_set:
            case ck_multiset:
              {
                break;
              }
            }

            t->traverse (m, vt, "value", "value");

            process_statement_columns (sc, statement_insert, versioned);

            os << strlit ("INSERT INTO " + qtable + sep) << endl;

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

            os << strlit ("VALUES" + sep) << endl;

            string values ("(");
            instance<query_parameters> qp (table);
            for (statement_columns::const_iterator b (sc.begin ()), i (b),
                   e (sc.end ()); i != e; ++i)
            {
              if (i != b)
              {
                values += ',';
                values += sep;
              }

              values += convert_to (qp->next (), i->type, *i->member);
            }
            values += ')';

            os << strlit (values) << ";"
               << endl;
          }

          // update_statement
          //
          if (smart)
          {
            os << "const char " << scope << "::" << endl
               << "update_statement[] =" << endl
               << strlit ("UPDATE " + qtable + sep) << endl
               << strlit ("SET" + sep) << endl;

            instance<query_parameters> qp (table);
            statement_columns sc;
            {
              bool f (false);                       // Imperfect forwarding.
              query_parameters* p (qp.get ());      // Imperfect forwarding.
              statement_kind sk (statement_update); // Imperfect forwarding.
              instance<object_columns> t (sk, f, sc, p);
              t->traverse (m, vt, "value", "value");
              process_statement_columns (sc, statement_update, versioned);
            }

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << strlit (c + (++i != e ? "," : "") + sep) << endl;
            }

            string where ("WHERE ");
            for (object_columns_list::iterator b (id_cols->begin ()), i (b);
                 i != id_cols->end (); ++i)
            {
              if (i != b)
                where += " AND ";

              where += quote_id (i->name) + "=" +
                convert_to (qp->next (), i->type, *i->member);
            }

            for (object_columns_list::iterator b (ik_cols->begin ()), i (b);
                 i != ik_cols->end (); ++i)
            {
              where += " AND " + quote_id (i->name) + "=" +
                convert_to (qp->next (), i->type, *i->member);
            }

            os << strlit (where) << ";"
               << endl;
          }

          // delete_statement
          //
          os << "const char " << scope << "::" << endl
             << "delete_statement[] =" << endl;

          if (inverse)
            os << strlit ("") << ";"
               << endl;
          else
          {
            instance<query_parameters> qp (table);

            os << strlit ("DELETE FROM " + qtable + " ") << endl;

            string where ("WHERE ");
            for (object_columns_list::iterator b (id_cols->begin ()), i (b);
                 i != id_cols->end (); ++i)
            {
              if (i != b)
                where += " AND ";

              where += quote_id (i->name) + "=" +
                convert_to (qp->next (), i->type, *i->member);
            }

            if (smart)
            {
              for (object_columns_list::iterator b (ik_cols->begin ()), i (b);
                   i != ik_cols->end (); ++i)
              {
                where += " AND " + quote_id (i->name) +
                  (ck == ck_ordered ? ">=" : "=") +
                  convert_to (qp->next (), i->type, *i->member);
              }
            }

            os << strlit (where) << ";"
               << endl;
          }
        }

        if (base)
          return;

        //
        // Functions.
        //

        // bind (cond_image_type)
        //
        if (smart)
        {
          os << "void " << scope << "::" << endl
             << "bind (" << bind_vector << " b," << endl
             << "const " << bind_vector << " id," << endl
             << "std::size_t id_size," << endl
             << "cond_image_type& c)"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "statement_kind sk (statement_select);"
             << "ODB_POTENTIALLY_UNUSED (sk);"
             << endl
             << "std::size_t n (0);"
             << endl;

          os << "// object_id" << endl
             << "//" << endl
             << "if (id != 0)" << endl
             << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
             << "n += id_size;" // Not in if for "id unchanged" optimization.
             << endl;

          // We don't need to update the bind index since this is the
          // last element.
          //
          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
              {
                os << "// index" << endl
                   << "//" << endl;
                instance<bind_member> bm (
                  "index_", "c", *it, "index_type", "index");
                bm->traverse (m);
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "// key" << endl
                 << "//" << endl;
              instance<bind_member> bm ("key_", "c", *kt, "key_type", "key");
              bm->traverse (m);
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              os << "// value" << endl
                 << "//" << endl;
              instance<bind_member> bm (
                "value_", "c", vt, "value_type", "value");
              bm->traverse (m);
              break;
            }
          }
          os << "}";
        }

        // bind (data_image_type)
        //
        {
          os << "void " << scope << "::" << endl
             << "bind (" << bind_vector << " b," << endl
             << "const " << bind_vector << " id," << endl
             << "std::size_t id_size," << endl
             << "data_image_type& d";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "using namespace " << db << ";"
             << endl
            // In the case of containers, insert and select column sets are
            // the same since we can't have inverse members as container
            // elements.
            //
             << "statement_kind sk (statement_select);"
             << "ODB_POTENTIALLY_UNUSED (sk);"
             << endl
             << "size_t n (0);"
             << endl;

          os << "// object_id" << endl
             << "//" << endl
             << "if (id != 0)" << endl
             << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
             << "n += id_size;" // Not in if for "id unchanged" optimization.
             << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
              {
                os << "// index" << endl
                   << "//" << endl;
                instance<bind_member> bm (
                  "index_", "d", *it, "index_type", "index");
                bm->traverse (m);
                os << "n++;" // Simple value.
                   << endl;
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "// key" << endl
                 << "//" << endl;
              instance<bind_member> bm ("key_", "d", *kt, "key_type", "key");
              bm->traverse (m);

              if (semantics::class_* c = composite_wrapper (*kt))
                os << "n += " << column_count (*c).total << "UL;"
                   << endl;
              else
                os << "n++;"
                   << endl;
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              break;
            }
          }

          // We don't need to update the bind index since this is the
          // last element.
          //
          os << "// value" << endl
             << "//" << endl;
          instance<bind_member> bm ("value_", "d", vt, "value_type", "value");
          bm->traverse (m);

          os << "}";
        }

        // bind (cond_image, data_image) (update)
        //
        if (smart)
        {
          os << "void " << scope << "::" << endl
             << "bind (" << bind_vector << " b," << endl
             << "const " << bind_vector << " id," << endl
             << "std::size_t id_size," << endl
             << "cond_image_type& c," << endl
             << "data_image_type& d";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "using namespace " << db << ";"
             << endl
            // Use insert instead of update to include read-only members.
            //
             << "statement_kind sk (statement_insert);"
             << "ODB_POTENTIALLY_UNUSED (sk);"
             << endl
             << "std::size_t n (0);"
             << endl;

          os << "// value" << endl
             << "//" << endl;
          instance<bind_member> bm ("value_", "d", vt, "value_type", "value");
          bm->traverse (m);

          if (semantics::class_* c = composite_wrapper (vt))
            os << "n += " << column_count (*c).total << "UL;"
               << endl;
          else
            os << "n++;"
               << endl;

          os << "// object_id" << endl
             << "//" << endl
             << "if (id != 0)" << endl
             << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
             << "n += id_size;" // Not in if for "id unchanged" optimization.
             << endl;

          // We don't need to update the bind index since this is the
          // last element.
          //
          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
              {
                os << "// index" << endl
                   << "//" << endl;
                instance<bind_member> bm (
                  "index_", "c", *it, "index_type", "index");
                bm->traverse (m);
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "// key" << endl
                 << "//" << endl;
              instance<bind_member> bm ("key_", "c", *kt, "key_type", "key");
              bm->traverse (m);
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              os << "// value" << endl
                 << "//" << endl;
              instance<bind_member> bm (
                "value_", "c", vt, "value_type", "value");
              bm->traverse (m);
              break;
            }
          }
          os << "}";
        }

        // grow ()
        //
        if (generate_grow)
        {
          size_t index (0);

          os << "void " << scope << "::" << endl
             << "grow (data_image_type& i," << endl
             << truncated_vector << " t";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "bool grew (false);"
             << endl;

          switch (ck)
          {
          case ck_ordered:
           {
              if (ordered)
              {
                os << "// index" << endl
                   << "//" << endl;
                instance<grow_member> gm (
                  index, "index_", *it, "index_type", "index");
                gm->traverse (m);
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "// key" << endl
                 << "//" << endl;
              instance<grow_member> gm (index, "key_", *kt, "key_type", "key");
              gm->traverse (m);
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              break;
            }
          }

          os << "// value" << endl
             << "//" << endl;
          instance<grow_member> gm (
            index, "value_", vt, "value_type", "value");
          gm->traverse (m);

          os << "if (grew)" << endl
             << "i.version++;"
             << "}";
        }

        // init (data_image)
        //
        if (!inverse)
        {
          os << "void " << scope << "::" << endl
             << "init (data_image_type& i," << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
                os << "index_type* j," << endl;
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "const key_type* k," << endl;
              break;
            }
          case ck_set:
          case ck_multiset:
            break;
          }

          os << "const value_type& v";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "statement_kind sk (statement_insert);"
             << "ODB_POTENTIALLY_UNUSED (sk);"
             << endl;

          if (generate_grow)
            os << "bool grew (false);"
               << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
              {
                os << "// index" << endl
                   << "//" << endl
                   << "if (j != 0)";

                instance<init_image_member> im (
                  "index_", "*j", *it, "index_type", "index");
                im->traverse (m);
              }
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "// key" << endl
                 << "//" << endl
                 << "if (k != 0)";

              instance<init_image_member> im (
                "key_", "*k", *kt, "key_type", "key");
              im->traverse (m);

              break;
            }
          case ck_set:
          case ck_multiset:
            {
              break;
            }
          }

          os << "// value" << endl
             << "//" << endl;
          {
            instance<init_image_member> im (
              "value_", "v", vt, "value_type", "value");
            im->traverse (m);
          }

          if (generate_grow)
            os << "if (grew)" << endl
               << "i.version++;";

          os << "}";
        }

        // init (cond_image)
        //
        if (smart)
        {
          os << "void " << scope << "::" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "init (cond_image_type& i, index_type j)"
                 << "{"
                 << "using namespace " << db << ";"
                 << endl
                 << "statement_kind sk (statement_select);"
                 << "ODB_POTENTIALLY_UNUSED (sk);"
                 << endl;

              instance<init_image_member> im (
                "index_", "j", *it, "index_type", "index");
              im->traverse (m);

              os << "}";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              // Need to handle growth.
              //
              // os << "init (data_image_type&, const key_type&);";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              // Need to handle growth.
              //
              // os << "init (data_image_type&, const value_type&);";
              break;
            }
          }

          os << endl;
        }

        // init (data)
        //
        os << "void " << scope << "::" << endl
           << "init (";

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
              os << "index_type& j," << endl;
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "key_type& k," << endl;
            break;
          }
        case ck_set:
        case ck_multiset:
          break;
        }

        os << "value_type& v," << endl;
        os << "const data_image_type& i," << endl
           << "database* db";

        if (versioned)
          os << "," << endl
             << "const schema_version_migration& svm";

        os << ")"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (db);"
           << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
            {
              os << "// index" << endl
                 << "//" << endl;

              instance<init_value_member> im (
                "index_", "j", *it, "index_type", "index");
              im->traverse (m);
            }

            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "// key" << endl
               << "//" << endl;

            instance<init_value_member> im (
              "key_", "k", *kt, "key_type", "key");
            im->traverse (m);

            break;
          }
        case ck_set:
        case ck_multiset:
          break;
        }

        os << "// value" << endl
           << "//" << endl;
        {
          // If the value is an object pointer, pass the id type as a
          // type override.
          //
          instance<init_value_member> im (
            "value_", "v", vt, "value_type", "value");
          im->traverse (m);
        }
        os << "}";

        // insert
        //
        {
          string ia, ka, va, da;

          if (!inverse)
          {
            ia = ordered ? " i" : "";
            ka = " k";
            va = " v";
            da = " d";
          }

          os << "void " << scope << "::" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "insert (index_type" << ia << ", " <<
                "const value_type&" << va << ", " <<
                "void*" << da << ")";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "insert (const key_type&" << ka << ", " <<
                "const value_type&" << va << ", " <<
                "void*" << da << ")";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              os << "insert (const value_type&" << va << ", " <<
                "void*" << da << ")";
              break;
            }
          }

          os << "{";

          if (!inverse)
          {
            os << "using namespace " << db << ";"
               << endl
               << "statements_type& sts (*static_cast< statements_type* > (d));"
               << "data_image_type& di (sts.data_image ());";

            if (versioned)
              os << "const schema_version_migration& svm (" <<
                "sts.version_migration ());";

            os << endl
               << "init (di, ";

            switch (ck)
            {
            case ck_ordered:
              {
                if (ordered)
                  os << "&i, ";
                break;
              }
            case ck_map:
            case ck_multimap:
              {
                os << "&k, ";
                break;
              }
            case ck_set:
            case ck_multiset:
              break;
            }

            os << "v" << (versioned ? ", svm" : "") << ");";

            os << endl
               << "if (sts.data_binding_test_version ())"
               << "{"
               << "const binding& id (sts.id_binding ());"
               << "bind (sts.data_bind (), id.bind, id.count, di" <<
              (versioned ? ", svm" : "") << ");"
               << "sts.data_binding_update_version ();"
               << "}"
               << "if (!sts.insert_statement ().execute ())" << endl
               << "throw object_already_persistent ();";
          }

          os << "}";
        }

        // update
        //
        if (smart)
        {
          os << "void " << scope << "::" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "update (index_type i, const value_type& v, void* d)";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              break;
            }
          }

          os << "{";

          os << "using namespace " << db << ";"
             << endl
             << "statements_type& sts (*static_cast< statements_type* > (d));"
             << "cond_image_type& ci (sts.cond_image ());"
             << "data_image_type& di (sts.data_image ());";

          if (versioned)
            os << "const schema_version_migration& svm (" <<
              "sts.version_migration ());";

          os << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "init (ci, i);";
              os << "init (di, 0, v" << (versioned ? ", svm" : "") << ");";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              //os << "init (di, 0, v);";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              //os << "init (di, v);";
              break;
            }
          }

          os << endl
             << "if (sts.update_binding_test_version ())"
             << "{"
             << "const binding& id (sts.id_binding ());"
             << "bind (sts.update_bind (), id.bind, id.count, ci, di" <<
            (versioned ? ", svm" : "") << ");"
             << "sts.update_binding_update_version ();"
             << "}";

          os << "if (sts.update_statement ().execute () == 0)" << endl
             << "throw object_not_persistent ();"
             << "}";
        }

        // select
        //
        os << "bool " << scope << "::" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            os << "select (index_type&" << (ordered ? " i" : "") <<
              ", value_type& v, void* d)";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "select (key_type& k, value_type& v, void* d)";
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "select (value_type& v, void* d)";
            break;
          }
        }

        os << "{"
           << "using namespace " << db << ";"
           << "using " << db << "::select_statement;" // Conflicts.
           << endl
           << "statements_type& sts (*static_cast< statements_type* > (d));"
           << "data_image_type& di (sts.data_image ());";

        if (versioned)
          os << "const schema_version_migration& svm (" <<
            "sts.version_migration ());";

        os << endl
           << "init (";

        // Extract current element.
        //
        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
              os << "i, ";
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "k, ";
            break;
          }
        case ck_set:
        case ck_multiset:
          break;
        }

        os << "v, di, &sts.connection ().database ()" <<
          (versioned ? ", svm" : "") << ");"
           << endl;

        init_value_extra ();

        // If we are loading an eager pointer, then the call to init
        // above executes other statements which potentially could
        // change the image, including the id.
        //
        if (eager_ptr)
        {
          os << "if (sts.data_binding_test_version ())"
             << "{"
             << "const binding& id (sts.id_binding ());"
             << "bind (sts.data_bind (), id.bind, id.count, di" <<
            (versioned ? ", svm" : "") << ");"
             << "sts.data_binding_update_version ();"
             << "}";
        }

        // Fetch next.
        //
        os << "select_statement& st (sts.select_statement ());"
           << "select_statement::result r (st.fetch ());";

        if (grow)
          os << endl
             << "if (r == select_statement::truncated)"
             << "{"
             << "grow (di, sts.select_image_truncated ()" <<
            (versioned ? ", svm" : "") << ");"
             << endl
             << "if (sts.data_binding_test_version ())"
             << "{"
            // Id cannot change.
            //
             << "bind (sts.data_bind (), 0, sts.id_binding ().count, di" <<
            (versioned ? ", svm" : "") << ");"
             << "sts.data_binding_update_version ();"
             << "st.refetch ();"
             << "}"
             << "}";

        os << "return r != select_statement::no_data;"
           << "}";

        // delete_
        //
        os << "void " << scope << "::" << endl
           << "delete_ (";

        if (smart)
        {
          switch (ck)
          {
          case ck_ordered:
            {
              os << "index_type i, ";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              break;
            }
          }
        }

        os << "void*" << (inverse ? "" : " d") << ")"
           << "{";

        if (!inverse)
        {
          os << "using namespace " << db << ";"
             << endl
             << "statements_type& sts (*static_cast< statements_type* > (d));";

          if (smart)
          {
            os << "cond_image_type& ci (sts.cond_image ());"
               << endl;

            switch (ck)
            {
            case ck_ordered:
              {
                os << "init (ci, i);";
                break;
              }
            case ck_map:
            case ck_multimap:
              {
                break;
              }
            case ck_set:
            case ck_multiset:
              {
                break;
              }
            }

            os << endl
               << "if (sts.cond_binding_test_version ())"
               << "{"
               << "const binding& id (sts.id_binding ());"
               << "bind (sts.cond_bind (), id.bind, id.count, ci);"
               << "sts.cond_binding_update_version ();"
               << "}";
          }

          os << "sts.delete_statement ().execute ();";
        }

        os << "}";

        // persist
        //
        if (!inverse)
        {
          os << "void " << scope << "::" << endl
             << "persist (const container_type& c," << endl
             << "statements_type& sts";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "functions_type& fs (sts.functions ());";

          if (versioned)
            os << "sts.version_migration (svm);";

          if (!smart && ck == ck_ordered)
            os << "fs.ordered_ = " << ordered << ";";

          os << "container_traits_type::persist (c, fs);"
             << "}";
        }

        // load
        //
        os << "void " << scope << "::" << endl
           << "load (container_type& c," << endl
           << "statements_type& sts";

        if (versioned)
          os << "," << endl
             << "const schema_version_migration& svm";

        os << ")"
           << "{"
           << "using namespace " << db << ";"
           << "using " << db << "::select_statement;" // Conflicts.
           << endl
           << "const binding& id (sts.id_binding ());"
           << endl
           << "if (sts.data_binding_test_version ())"
           << "{"
           << "bind (sts.data_bind (), id.bind, id.count, sts.data_image ()" <<
          (versioned ? ", svm" : "") << ");"
           << "sts.data_binding_update_version ();"
           << "}"
          // We use the id binding directly so no need to check cond binding.
          //
           << "select_statement& st (sts.select_statement ());"
           << "st.execute ();"
           << "auto_result ar (st);";

        // If we are loading eager object pointers, we may need to cache
        // the result since we will be loading other objects.
        //
        if (eager_ptr)
          cache_result ("st");

        os << "select_statement::result r (st.fetch ());";

        if (grow)
          os << endl
             << "if (r == select_statement::truncated)"
             << "{"
             << "data_image_type& di (sts.data_image ());"
             << "grow (di, sts.select_image_truncated ()" <<
            (versioned ? ", svm" : "") << ");"
             << endl
             << "if (sts.data_binding_test_version ())"
             << "{"
            // Id cannot change.
            //
             << "bind (sts.data_bind (), 0, id.count, di" <<
            (versioned ? ", svm" : "") << ");"
             << "sts.data_binding_update_version ();"
             << "st.refetch ();"
             << "}"
             << "}";

        os << "bool more (r != select_statement::no_data);"
           << endl
           << "functions_type& fs (sts.functions ());";

        if (versioned)
          os << "sts.version_migration (svm);";

        if (!smart && ck == ck_ordered)
          os << "fs.ordered_ = " << ordered << ";";

        os << "container_traits_type::load (c, more, fs);"
           << "}";

        // update
        //
        if (!(inverse || readonly (member_path_, member_scope_)))
        {
          os << "void " << scope << "::" << endl
             << "update (const container_type& c," << endl
             << "statements_type& sts";

          if (versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "functions_type& fs (sts.functions ());";

          if (versioned)
            os << "sts.version_migration (svm);";

          if (!smart && ck == ck_ordered)
            os << "fs.ordered_ = " << ordered << ";";

          os << "container_traits_type::update (c, fs);"
             << "}";
        }

        // erase
        //
        if (!inverse)
        {
          os << "void " << scope << "::" << endl
             << "erase (";

          if (smart)
            os << "const container_type* c, ";

          os << "statements_type& sts)"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "functions_type& fs (sts.functions ());";

          if (!smart && ck == ck_ordered)
            os << "fs.ordered_ = " << ordered << ";";

          os << "container_traits_type::erase (" << (smart ? "c, " : "") << "fs);"
             << "}";
        }
      }

    protected:
      string scope_;
      semantics::class_& c_;
    };

    // Extra statement cache members for containers.
    //
    struct container_cache_members: object_members_base, virtual context
    {
      typedef container_cache_members base;

      container_cache_members ()
          : object_members_base (true, false, false)
      {
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type& c)
      {
        bool smart (!context::inverse (m, "value") &&
                    !unordered (m) &&
                    container_smart (c));

        string traits (flat_prefix_ + public_name (m) + "_traits");

        os << db << "::" << (smart ? "smart_" : "") <<
          "container_statements_impl< " << traits << " > " <<
          flat_prefix_ << m.name () << ";";
      }
    };

    struct container_cache_init_members: object_members_base, virtual context
    {
      typedef container_cache_init_members base;

      container_cache_init_members ()
          : object_members_base (true, false, false), first_ (true)
      {
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type&)
      {
        if (first_)
        {
          os << endl
             << ": ";
          first_ = false;
        }
        else
          os << "," << endl
             << "  ";

        os << flat_prefix_ << m.name () << " (c, id";
        extra_members ();
        os << ")";
      }

      virtual void
      extra_members () {}

    protected:
      bool first_;
    };

    // Extra statement cache members for sections.
    //
    struct section_cache_members: virtual context
    {
      typedef section_cache_members base;

      virtual void
      traverse (user_section& s)
      {
        string traits (public_name (*s.member) + "_traits");

        os << db << "::" << "section_statements< " <<
          class_fq_name (*s.object) << ", " << traits << " > " <<
          s.member->name () << ";";
      }
    };

    struct section_cache_init_members: virtual context
    {
      typedef section_cache_init_members base;

      section_cache_init_members (bool first): first_ (first) {}

      virtual void
      traverse (user_section& s)
      {
        if (first_)
        {
          os << endl
             << ": ";
          first_ = false;
        }
        else
          os << "," << endl
             << "  ";

        os << s.member->name () << " (c, im, idim, id, idv";
        extra_members ();
        os << ")";
      }

      virtual void
      extra_members () {}

    protected:
      bool first_;
    };

    // Calls for container members.
    //
    struct container_calls: object_members_base, virtual context
    {
      typedef container_calls base;

      enum call_type
      {
        persist_call,
        load_call,
        update_call,
        erase_obj_call,
        erase_id_call,
        section_call
      };

      container_calls (call_type call, object_section* section = 0)
          : object_members_base (true, false, true, false, section),
            call_ (call),
            obj_prefix_ ("obj"),
            modifier_ (0)
      {
      }

      virtual bool
      section_test (data_member_path const& mp)
      {
        object_section& s (section (mp));

        // Include eager loaded members into the main section for
        // load calls.
        //
        return section_ == 0 ||
          *section_ == s ||
          (call_ == load_call &&
           *section_ == main_section &&
           !s.separate_load ());
      }

      virtual void
      traverse_pointer (semantics::data_member&, semantics::class_&)
      {
        // We don't want to traverse composite id.
      }

      virtual void
      traverse_composite_wrapper (semantics::data_member* m,
                                  semantics::class_& c,
                                  semantics::type* w)
      {
        if (m == 0 || call_ == erase_id_call || modifier_ != 0)
        {
          object_members_base::traverse_composite (m, c);
          return;
        }

        // Get this member using the accessor expression.
        //
        member_access& ma (
          m->get<member_access> (call_ == load_call ? "set" : "get"));

        // We don't support by-value modifiers for composite values
        // with containers. However, at this point we don't know
        // whether this composite value has any containers. So we
        // are just going to set a flag that can be checked in
        // traverse_container() below.
        //
        if (ma.placeholder ())
        {
          modifier_ = &ma;
          object_members_base::traverse_composite (m, c);
          modifier_ = 0;
          return;
        }

        string old_op (obj_prefix_);
        string old_f (from_);
        obj_prefix_.clear ();

        // If this member is const and we have a synthesized direct
        // access, then cast away constness. Otherwise, we assume
        // that the user-provided expression handles this.
        //
        bool cast (
          call_ == load_call && ma.direct () && const_type (m->type ()));
        if (cast)
          obj_prefix_ = "const_cast< " + member_ref_type (*m, false) +
            " > (\n";

        obj_prefix_ += ma.translate (old_op);

        if (cast)
          obj_prefix_ += ")";

        // If this is not a synthesized expression, then store its
        // location which we will output later for easier error
        // tracking.
        //
        if (!ma.synthesized)
          from_ += "// From " + location_string (ma.loc, true) + "\n";

        // If this is a wrapped composite value, then we need to "unwrap" it.
        //
        if (w != 0)
        {
          semantics::names* hint;
          semantics::type& t (utype (*m, hint));

          // Because we cannot have nested containers, m.type () should
          // be the same as w.
          //
          assert (&t == w);

          obj_prefix_ = "wrapper_traits< " + t.fq_name (hint) + " >::" +
            (call_ == load_call ? "set_ref" : "get_ref") +
            " (\n" + obj_prefix_ + ")";
        }

        object_members_base::traverse_composite (m, c);
        from_ = old_f;
        obj_prefix_ = old_op;
      }

      virtual void
      traverse_container (semantics::data_member& m, semantics::type& c)
      {
        using semantics::type;

        bool inverse (context::inverse (m, "value"));
        bool smart (!inverse && !unordered (m) && container_smart (c));
        bool versioned (context::versioned (m));

        // In certain cases we don't need to do anything.
        //
        if ((call_ != load_call && inverse) ||
            (call_ == section_call && !smart) ||
            (call_ == update_call && readonly (member_path_, member_scope_)))
          return;

        string const& name (m.name ());
        string sts_name (flat_prefix_ + name);
        string traits (flat_prefix_ + public_name (m) + "_traits");

        os << "// " << member_prefix_ << m.name () << endl
           << "//" << endl;

        // Get this member using the accessor expression.
        //
        string var;
        member_access& ma (
          m.get<member_access> (call_ == load_call ? "set" : "get"));

        // We don't support by-value modifiers for composite values
        // with containers.
        //
        if (call_ == load_call && modifier_ != 0)
        {
          error (modifier_->loc) << "by-value modification of a composite "
                                 << "value with container is not supported"
                                 << endl;
          info (m.location ()) << "container member is defined here" << endl;
          throw operation_failed ();
        }

        // If the member is soft- added or deleted, check the version.
        //
        unsigned long long av (added (member_path_));
        unsigned long long dv (deleted (member_path_));

        // If the addition/deletion version is the same as the section's,
        // then we don't need the test.
        //
        if (user_section* s = dynamic_cast<user_section*> (section_))
        {
          if (av == added (*s->member))
            av = 0;

          if (dv == deleted (*s->member))
            dv = 0;
        }

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

        if (call_ != erase_id_call && (call_ != erase_obj_call || smart))
        {
          os << "{";

          // Output stored locations, if any.
          //
          if (!ma.placeholder ())
            os << from_;

          // If this is not a synthesized expression, then output its
          // location for easier error tracking.
          //
          if (!ma.synthesized)
            os << "// From " << location_string (ma.loc, true) << endl;

          // See if we are modifying via a reference or proper modifier.
          //
          if (ma.placeholder ())
            os << member_val_type (m, false, "v") << ";"
               << endl;
          else
          {
            // VC++ cannot grok the constructor syntax.
            //
            os << member_ref_type (m, call_ != load_call, "v") << " =" << endl
               << "  ";

            // If this member is const and we have a synthesized direct
            // access, then cast away constness. Otherwise, we assume
            // that the user-provided expression handles this.
            //
            bool cast (
              call_ == load_call && ma.direct () && const_type (m.type ()));
            if (cast)
              os << "const_cast< " << member_ref_type (m, false) <<
                " > (" << endl;

            os << ma.translate (obj_prefix_);

            if (cast)
              os << ")";

            os << ";"
               << endl;
          }

          var = "v";

          semantics::names* hint;
          semantics::type& t (utype (m, hint));

          // If this is a wrapped container, then we need to "unwrap" it.
          //
          if (wrapper (t))
          {
            var = "wrapper_traits< " + t.fq_name (hint) + " >::" +
              (call_ == load_call ? "set_ref" : "get_ref") + " (" + var + ")";
          }
        }

        switch (call_)
        {
        case persist_call:
          {
            os << traits << "::persist (" << endl
               << var << "," << endl
               << "esc." << sts_name;

            if (versioned)
              os << "," << endl
                 << "svm";

            os << ");";
            break;
          }
        case load_call:
          {
            os << traits << "::load (" << endl
               << var << "," << endl
               << "esc." << sts_name;

            if (versioned)
              os << "," << endl
                 << "svm";

            os << ");";
            break;
          }
        case update_call:
          {
            os << traits << "::update (" << endl
               << var << "," << endl
               << "esc." << sts_name;

            if (versioned)
              os << "," << endl
                 << "svm";

            os << ");";
            break;
          }
        case erase_obj_call:
          {
            os << traits << "::erase (" << endl;

            if (smart)
              os << "&" << var << "," << endl;

            os << "esc." << sts_name << ");"
               << endl;
            break;
          }
        case erase_id_call:
          {
            os << traits << "::erase (" << endl;

            if (smart)
              os << "0," << endl;

            os << "esc." << sts_name << ");"
               << endl;
            break;
          }
        case section_call:
          {
            os << "if (" << traits << "::container_traits_type::changed (" <<
              var << "))" << endl
               << "s.reset (true, true);"; // loaded, changed
            break;
          }
        }

        if (call_ != erase_id_call && (call_ != erase_obj_call || smart))
        {
          // Call the modifier if we are using a proper one.
          //
          if (ma.placeholder ())
          {
            os << endl
               << from_;

            // If this is not a synthesized expression, then output its
            // location for easier error tracking.
            //
            if (!ma.synthesized)
              os << "// From " << location_string (ma.loc, true) << endl;

            os << ma.translate (obj_prefix_, "v") << ";";
          }

          os << "}";
        }
      }

    protected:
      call_type call_;
      string obj_prefix_;
      string from_;
      member_access* modifier_;
    };

    //
    //
    struct section_traits: traversal::class_, virtual context
    {
      typedef section_traits base;

      section_traits (semantics::class_& c)
          : c_ (c),
            scope_ ("access::object_traits_impl< " + class_fq_name (c) +
                    ", id_" + db.string () + " >")
      {
      }

      // Additional code that need to be executed following the call to
      // init_value().
      //
      virtual void
      init_value_extra ()
      {
      }

      virtual void
      process_statement_columns (statement_columns&,
                                 statement_kind,
                                 bool /*dynamic*/)
      {
      }

      virtual void
      section_extra (user_section&)
      {
      }

      // Returning "1" means increment by one.
      //
      virtual string
      optimistic_version_increment (semantics::data_member&)
      {
        return "1";
      }

      virtual string
      update_statement_extra (user_section&)
      {
        return "";
      }

      virtual void
      traverse (user_section& s)
      {
        using semantics::class_;
        using semantics::data_member;

        data_member& m (*s.member);

        class_* poly_root (polymorphic (c_));
        bool poly (poly_root != 0);
        bool poly_derived (poly && poly_root != &c_);
        class_* poly_base (poly_derived ? &polymorphic_base (c_) : 0);

        data_member* opt (optimistic (c_));

        // Treat the special version update sections as abstract in reuse
        // inheritance.
        //
        bool reuse_abst (!poly &&
                         (abstract (c_) ||
                          s.special == user_section::special_version));

        bool load (s.total != 0 && s.separate_load ());
        bool load_con (s.containers && s.separate_load ());
        bool load_opt (s.optimistic () && s.separate_load ());

        bool update (s.total != s.inverse + s.readonly); // Always separate.
        bool update_con (s.readwrite_containers);
        bool update_opt (s.optimistic () && (s.readwrite_containers || poly));

        // Don't generate anything for empty sections.
        //
        if (!(load || load_con || load_opt ||
              update || update_con || update_opt))
          return;

        // If we are adding a new section to a derived class in an optimistic
        // polymorphic hierarchy, then pretend it inherits from the special
        // version update section.
        //
        user_section* rs (0);
        if (opt != 0)
        {
          // Skip overrides and get to the new section if polymorphic.
          //
          for (rs = &s; poly && rs->base != 0; rs = rs->base) ;

          if (rs != 0)
          {
            if (rs->object != &opt->scope ())
              rs->base = &(poly ? poly_root : &opt->scope ())->
                get<user_sections> ("user-sections").back ();
            else
              rs = 0;
          }
        }

        string name (public_name (m) + "_traits");
        string scope (scope_ + "::" + name);

        os << "// " << m.name () << endl
           << "//" << endl
           << endl;

        // bind (id, image_type)
        //
        if (load || load_opt || update || update_opt)
        {
          os << "std::size_t " << scope << "::" << endl
             << "bind (" << bind_vector << " b," << endl
             << "const " << bind_vector << (reuse_abst ? "," : " id,") << endl
             << "std::size_t" << (reuse_abst ? "," : " id_size,") << endl
             << "image_type& i," << endl
             << db << "::statement_kind sk";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (sk);";

          if (s.versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);";

          os << endl
             << "using namespace " << db << ";"
             << endl
             << "std::size_t n (0);"
             << endl;

          // Bind reuse base. It is always first and we never ask it
          // to bind id(+ver).
          //
          if (s.base != 0 && !poly_derived)
          {
            user_section& b (*s.base);

            bool load (b.total != 0 && b.separate_load ());
            bool load_opt (b.optimistic () && b.separate_load ());

            bool update (b.total != b.inverse + b.readonly);

            if (load || load_opt || update)
              os << "// " << class_name (*b.object) << endl
                 << "//" << endl
                 << "n += object_traits_impl< " << class_fq_name (*b.object) <<
                ", id_" << db << " >::" << public_name (*b.member) <<
                "_traits::bind (" << endl
                 << "b, 0, 0, i, sk" << (b.versioned ? ", svm" : "") << ");"
                 << endl;
          }

          // Bind members.
          //
          {
            instance<bind_member> bm ("", "", &s);
            traversal::names n (*bm);
            names (c_, n);
          }

          // Bind polymorphic image chain for the select statement.
          //
          if (s.base != 0 && poly_derived && s.separate_load ())
          {
            // Find the next base that has something to load, if any.
            //
            user_section* b (s.base);
            string acc (".base");
            for (class_* bo (poly_base);; bo = &polymorphic_base (*bo))
            {
              if (b->object == bo)
              {
                if (b->total != 0 || b->optimistic ())
                  break;

                b = b->base;
                if (b == 0 || !polymorphic (*b->object))
                {
                  b = 0;
                  break;
                }
              }
              acc += "->base";
            }

            if (b != 0)
              os << "// " << class_name (*b->object) << endl
                 << "//" << endl
                 << "if (sk == statement_select)" << endl
                 << "n += object_traits_impl< " << class_fq_name (*b->object) <<
                ", id_" << db << " >::" << public_name (*b->member) <<
                "_traits::bind (" << endl
                 << "b + n, 0, 0, *i" << acc << ", sk" <<
                (b->versioned ? ", svm" : "") << ");"
                 << endl;
          }

          if (!reuse_abst)
            os << "// object_id" << endl
               << "//" << endl
               << "if (id != 0)" << endl
               << "std::memcpy (&b[n], id, id_size * sizeof (id[0]));"
               << "n += id_size;" // Not in if for "id unchanged" optimization.
               << endl;

          os << "return n;"
             << "}";
        }

        // grow ()
        //
        if (generate_grow && (load || load_opt))
        {
          os << "bool " << scope << "::" << endl
             << "grow (image_type& i," << endl
             << truncated_vector << " t";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (i);"
             << "ODB_POTENTIALLY_UNUSED (t);";

          if (s.versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);";

          os << endl
             << "bool grew (false);"
             << endl;

          size_t index (0);

          if (s.base != 0 && !poly_derived)
          {
            user_section& b (*s.base);

            bool load (b.total != 0);
            bool load_opt (b.optimistic ());

            if (load || load_opt)
            {
              os << "// " << class_name (*b.object) << endl
                 << "//" << endl
                 << "grew = object_traits_impl< " << class_fq_name (*b.object) <<
                ", id_" << db << " >::" << public_name (*b.member) <<
                "_traits::grow (i, t" << (b.versioned ? ", svm" : "") << ");"
                 << endl;

              index += b.total + (load_opt ? 1 : 0);
            }
          }

          {
            user_section* ps (&s);
            instance<grow_member> gm (index, "", ps);
            traversal::names n (*gm);
            names (c_, n);
          }

          // Grow polymorphic image chain.
          //
          if (s.base != 0 && poly_derived)
          {
            // Find the next base that has something to load, if any.
            //
            user_section* b (s.base);
            string acc (".base");
            size_t cols;
            for (class_* bo (poly_base);; bo = &polymorphic_base (*bo))
            {
              if (b->object == bo)
              {
                cols = b->total + (b->optimistic () ? 1 : 0);
                if (cols != 0)
                  break;

                b = b->base;
                if (b == 0 || !polymorphic (*b->object))
                {
                  b = 0;
                  break;
                }
              }
              acc += "->base";
            }

            if (b != 0)
              os << "// " << class_name (*b->object) << endl
                 << "//" << endl
                 << "if (object_traits_impl< " << class_fq_name (*b->object) <<
                ", id_" << db << " >::" << public_name (*b->member) <<
                "_traits::grow (" << endl
                 << "*i" << acc << ", t + " << cols << "UL" <<
                (b->versioned ? ", svm" : "") << "))" << endl
                 << "i" << acc << "->version++;"
                 << endl;
          }

          os << "return grew;" << endl
             << "}";
        }

        // init (object, image)
        //
        if (load)
        {
          os << "void " << scope << "::" << endl
             << "init (object_type& o," << endl
             << "const image_type& i," << endl
             << "database* db";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (db);";

          if (s.versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);";

          os << endl;

          if (s.base != 0)
          {
            if (!poly_derived)
            {
              user_section& b (*s.base);

              bool load (b.total != 0);

              if (load)
                os << "// " << class_name (*b.object) << endl
                   << "//" << endl
                   << "object_traits_impl< " << class_fq_name (*b.object) <<
                  ", id_" << db << " >::" << public_name (*b.member) <<
                  "_traits::init (o, i, db" <<
                  (b.versioned ? ", svm" : "") << ");"
                   << endl;
            }
            else
            {
              // Find the next base that has something to load, if any.
              //
              user_section* b (s.base);
              string acc (".base");
              for (class_* bo (poly_base);; bo = &polymorphic_base (*bo))
              {
                if (b->object == bo)
                {
                  if (b->total != 0)
                    break;

                  b = b->base;
                  if (b == 0 || !polymorphic (*b->object))
                  {
                    b = 0;
                    break;
                  }
                }
                acc += "->base";
              }

              if (b != 0)
                os << "// " << class_name (*b->object) << endl
                   << "//" << endl
                   << "object_traits_impl< " << class_fq_name (*b->object) <<
                  ", id_" << db << " >::" << public_name (*b->member) <<
                  "_traits::init (" << endl
                   << "o, *i" << acc << ", db" <<
                  (b->versioned ? ", svm" : "") << ");"
                   << endl;
            }
          }

          {
            instance<init_value_member> iv ("", "", true, &s);
            traversal::names n (*iv);
            names (c_, n);
          }

          os << "}";
        }

        // init (image, object)
        //
        if (update)
        {
          os << (generate_grow ? "bool " : "void ") << scope << "::" << endl
             << "init (image_type& i," << endl
             << "const object_type& o";

          if (s.versioned)
            os << "," << endl
               << "const schema_version_migration& svm";

          os << ")"
             << "{";

          if (s.versioned)
            os << "ODB_POTENTIALLY_UNUSED (svm);"
               << endl;

          os << "using namespace " << db << ";"
             << endl
             << "statement_kind sk (statement_insert);"
             << "ODB_POTENTIALLY_UNUSED (sk);"
             << endl;

          // There is no call to init_image_pre() here (which calls the
          // copy callback for some databases) since we are not going to
          // touch any of the members that were loaded by query.

          if (generate_grow)
            os << "bool grew (false);"
               << endl;

          if (s.base != 0 && !poly_derived)
          {
            user_section& b (*s.base);

            bool update (b.total != b.inverse + b.readonly);

            if (update)
              os << "// " << class_name (*b.object) << endl
                 << "//" << endl
                 << (generate_grow ? "grew = " : "") <<
                "object_traits_impl< " << class_fq_name (*b.object) <<
                ", id_" << db << " >::" << public_name (*b.member) <<
                "_traits::init (i, o" << (b.versioned ? ", svm" : "") << ");"
                 << endl;
          }

          {
            instance<init_image_member> ii ("", "", &s);
            traversal::names n (*ii);
            names (c_, n);
          }

          if (generate_grow)
            os << "return grew;";

          os << "}";
        }

        // The rest does not apply to reuse-abstract sections.
        //
        if (reuse_abst)
        {
          section_extra (s);
          return;
        }

        string sep (s.versioned ? "\n" : " ");

        // Schema name as a string literal or empty.
        //
        string schema_name (options.schema_name ()[db]);
        if (!schema_name.empty ())
          schema_name = strlit (schema_name);

        // Statements.
        //
        qname table (table_name (c_));
        string qtable (quote_id (table));

        instance<object_columns_list> id_cols;
        id_cols->traverse (*id_member (c_));

        // select_statement
        //
        if (load || load_opt)
        {
          size_t depth (poly_derived ? polymorphic_depth (c_) : 1);

          statement_columns sc;
          {
            statement_kind sk (statement_select); // Imperfect forwarding.
            object_section* ps (&s);              // Imperfect forwarding.
            instance<object_columns> t (qtable, sk, sc, depth, ps);
            t->traverse (c_);
            process_statement_columns (sc, statement_select, s.versioned);
          }

          os << "const char " << scope << "::" << endl
             << "select_statement[] =" << endl
             << strlit ("SELECT" + sep) << endl;

          for (statement_columns::const_iterator i (sc.begin ()),
                 e (sc.end ()); i != e;)
          {
            string const& c (i->column);
            os << strlit (c + (++i != e ? "," : "") + sep) << endl;
          }

          os << strlit ("FROM " + qtable + sep) << endl;

          // Join polymorphic bases.
          //
          if (depth != 1 && s.base != 0)
          {
            bool f (false);             //@@ (im)perfect forwarding
            size_t d (depth - 1);       //@@ (im)perfect forward.
            instance<polymorphic_object_joins> j (c_, f, d, "", s.base);
            j->traverse (*poly_base);

            for (strings::const_iterator i (j->begin ()); i != j->end (); ++i)
              os << strlit (*i + sep) << endl;
          }

          // Join tables of inverse members belonging to this section.
          //
          {
            bool f (false);          // @@ (im)perfect forwarding
            object_section* ps (&s); // @@ (im)perfect forwarding
            instance<object_joins> j (c_, f, depth, ps);
            j->traverse (c_);

            for (strings::const_iterator i (j->begin ()); i != j->end (); ++i)
              os << strlit (*i + sep) << endl;
          }

          string where ("WHERE ");
          instance<query_parameters> qp (table);
          for (object_columns_list::iterator b (id_cols->begin ()), i (b);
               i != id_cols->end (); ++i)
          {
            if (i != b)
              where += " AND ";

            where += qtable + "." + quote_id (i->name) + "=" +
              convert_to (qp->next (), i->type, *i->member);
          }

          os << strlit (where) << ";"
             << endl;
        }

        // update_statement
        //
        if (update || update_opt)
        {
          instance<query_parameters> qp (table);

          statement_columns sc;
          {
            query_parameters* p (qp.get ());      // Imperfect forwarding.
            statement_kind sk (statement_update); // Imperfect forwarding.
            object_section* ps (&s);              // Imperfect forwarding.
            instance<object_columns> t (sk, sc, p, ps);
            t->traverse (c_);
            process_statement_columns (sc, statement_update, s.versioned);
          }

          os << "const char " << scope << "::" << endl
             << "update_statement[] =" << endl
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

          string extra (update_statement_extra (s));

          if (!extra.empty ())
            os << strlit (extra + sep) << endl;

          string where ("WHERE ");
          for (object_columns_list::iterator b (id_cols->begin ()), i (b);
               i != id_cols->end (); ++i)
          {
            if (i != b)
              where += " AND ";

            where += quote_id (i->name) + "=" +
              convert_to (qp->next (), i->type, *i->member);
          }

          if (s.optimistic ()) // Note: not update_opt.
          {
            where += " AND " + column_qname (*opt, column_prefix ()) + "=" +
              convert_to (qp->next (), *opt);
          }

          os << strlit (where) << ";"
             << endl;
        }

        // load ()
        //
        if (load || load_opt || load_con)
        {
          os << "void " << scope << "::" << endl
             << "load (extra_statement_cache_type& esc, object_type& obj" <<
            (poly ? ", bool top" : "") << ")"
             << "{";

          if (poly)
            os << "ODB_POTENTIALLY_UNUSED (top);"
               << endl;

          if (s.versioned || s.versioned_containers)
            os << "const schema_version_migration& svm (" << endl
               << "esc." << m.name () << ".version_migration (" <<
              schema_name << "));"
               << endl;

          // Load values, if any.
          //
          if (load || load_opt)
          {
            // The SELECT statement for the top override loads all the
            // values.
            //
            if (poly)
              os << "if (top)"
                 << "{";

            // Note that we don't use delayed load machinery here. While
            // a section can definitely contain self-referencing pointers,
            // loading such a pointer won't mess up the data members in the
            // image that we care about. It also holds true for streaming
            // result, since the bindings are different.

            os << "using namespace " << db << ";"
               << "using " << db << "::select_statement;" // Conflicts.
               << endl
               << "statements_type& sts (esc." << m.name () << ");"
               << endl
               << "image_type& im (sts.image ());"
               << "binding& imb (sts.select_image_binding ());"
               << endl;

            // For the polymorphic case, instead of storing an array of
            // versions as we do for objects, we will add all the versions
            // up and use that as a cumulative image chain version. If you
            // meditate a bit on that, you will realize that it will work
            // (hint: versions can only increase).
            //
            string ver;
            string ver_decl;

            if (s.base != 0 && poly_derived)
            {
              ver = "imv";
              ver_decl = "std::size_t imv (im.version";

              user_section* b (s.base);
              string acc ("im.base");
              for (class_* bo (poly_base);; bo = &polymorphic_base (*bo))
              {
                if (b->object == bo)
                {
                  if (b->total != 0 || b->optimistic ())
                    ver_decl += " +\n" + acc + "->version";

                  b = b->base;
                  if (b == 0 || !polymorphic (*b->object))
                  {
                    b = 0;
                    break;
                  }
                }
                acc += "->base";
              }

              ver_decl += ")";

              os << ver_decl << ";"
                 << endl;
            }
            else
              ver = "im.version";

            os << "if (" << ver << " != sts.select_image_version () ||" << endl
               << "imb.version == 0)"
               << "{"
               << "bind (imb.bind, 0, 0, im, statement_select" <<
              (s.versioned ? ", svm" : "") << ");"
               << "sts.select_image_version (" << ver << ");"
               << "imb.version++;"
               << "}";

            // Id binding is assumed initialized and bound.
            //
            os << "select_statement& st (sts.select_statement ());";

            // The statement can be dynamically empty.
            //
            if (s.versioned)
              os << "if (!st.empty ())"
                 << "{";

            os << "st.execute ();"
               << "auto_result ar (st);"
               << "select_statement::result r (st.fetch ());"
               << endl;

            os << "if (r == select_statement::no_data)" << endl
               << "throw object_not_persistent ();"
               << endl;

            if (grow (c_, &s))
            {
              os << "if (r == select_statement::truncated)"
                 << "{"
                 << "if (grow (im, sts.select_image_truncated ()" <<
                (s.versioned ? ", svm" : "") << "))" << endl
                 << "im.version++;"
                 << endl;

              // The same logic as above.
              //
              if (s.base != 0 && poly_derived)
                os << ver_decl << ";"
                   << endl;

              os << "if (" << ver << " != sts.select_image_version ())"
                 << "{"
                 << "bind (imb.bind, 0, 0, im, statement_select" <<
                (s.versioned ? ", svm" : "") << ");"
                 << "sts.select_image_version (" << ver << ");"
                 << "imb.version++;"
                 << "st.refetch ();"
                 << "}"
                 << "}";
            }

            if (opt != 0) // Not load_opt, we do it in poly-derived as well.
            {
              member_access& ma (opt->get<member_access> ("get"));

              if (!ma.synthesized)
                os << "// From " << location_string (ma.loc, true) << endl;

              os << "if (";

              if (poly_derived)
              {
                os << "root_traits::version (*im.base";
                for (class_* b (poly_base);
                     b != poly_root;
                     b = &polymorphic_base (*b))
                  os << "->base";
                os << ")";
              }
              else
                os << "version (im)";

              os << " != " << ma.translate ("obj") << ")" << endl
                 << "throw object_changed ();"
                 << endl;
            }

            if (load)
            {
              os << "init (obj, im, &sts.connection ().database ()" <<
                (s.versioned ? ", svm" : "") << ");";
              init_value_extra (); // Stream results, etc.
              os << endl;
            }

            if (s.versioned)
              os << "}"; // if (!st.empty ())

            if (poly)
              os << "}"; // if (top)
          }

          // Call base to load its containers, if this is an override.
          //
          if (poly_derived && s.base != 0)
          {
            user_section* b (s.base);
            for (class_* bo (poly_base);; bo = &polymorphic_base (*bo))
            {
              if (b->object == bo)
              {
                // If we don't have any values of our own but out base
                // does, then allow it to load them.
                //
                if (b->containers ||
                    (!load && (b->total != 0 || b->optimistic ())))
                  break;

                b = b->base;
                if (b == 0 || !polymorphic (*b->object))
                {
                  b = 0;
                  break;
                }
              }
            }

            // This one is tricky: ideally we would do a direct call to
            // the base's load() (which may not be our immediate base,
            // BTW) but there is no easy way to resolve base's extra
            // statements from ours. So, instead, we are going to go
            // via the dispatch machinery which requires a connection
            // rather than statements. Not the most efficient way but
            // simple.

            // Find the "previous" override by starting the search from
            // our base.
            //
            if (b != 0)
            {
              // Note that here we are using the base section index to
              // handle the special version update base.
              //
              os << "info.base->find_section_load (" << b->index << "UL) (" <<
                "esc." << m.name () << ".connection (), obj, " <<
                // If we don't have any values of our own, then allow the
                // base load its.
                //
                (load ? "false" : "top") << ");"
                 << endl;
            }
          }

          // Load our containers, if any.
          //
          if (s.containers)
          {
            instance<container_calls> t (container_calls::load_call, &s);
            t->traverse (c_);
          }

          os << "}";
        }

        // update ()
        //
        if (update || update_opt || update_con)
        {
          os << "void " << scope << "::" << endl
             << "update (extra_statement_cache_type& esc, " <<
            "const object_type& obj" <<
            (poly_derived && s.base != 0 ? ", bool base" : "") << ")"
             << "{";

          // Call base if this is an override.
          //
          if (poly_derived && s.base != 0)
          {
            user_section* b (s.base);
            for (class_* bo (poly_base);; bo = &polymorphic_base (*bo))
            {
              if (b->object == bo)
              {
                if (b->total != b->inverse + b->readonly ||
                    b->readwrite_containers ||
                    (poly && b->optimistic ()))
                  break;

                b = b->base;
                if (b == 0 || !polymorphic (*b->object))
                {
                  b = 0;
                  break;
                }
              }
            }

            // The same (tricky) logic as in load(). Note that here we are
            // using the base section index to handle the special version
            // update base.
            //
            if (b != 0)
              os << "if (base)" << endl
                 << "info.base->find_section_update (" << b->index <<
                "UL) (esc." << m.name () << ".connection (), obj);"
                 << endl;
            else
              os << "ODB_POTENTIALLY_UNUSED (base);"
                 << endl;
          }

          if (s.versioned || s.readwrite_versioned_containers)
            os << "const schema_version_migration& svm (" << endl
               << "esc." << m.name () << ".version_migration (" <<
              schema_name << "));"
               << endl;

          // Update values, if any.
          //
          if (update || update_opt)
          {
            os << "using namespace " << db << ";"
               << "using " << db << "::update_statement;" // Conflicts.
               << endl
               << "statements_type& sts (esc." << m.name () << ");"
               << endl
               << "image_type& im (sts.image ());"
               << "const binding& id (sts.idv_binding ());" // id+version
               << "binding& imb (sts.update_image_binding ());"
               << endl;

            if (update)
            {
              if (generate_grow)
                os << "if (";

              os << "init (im, obj" << (s.versioned ? ", svm" : "") << ")";

              if (generate_grow)
                os << ")" << endl
                   << "im.version++";

              os << ";"
                 << endl;
            }

            os << "if (im.version != sts.update_image_version () ||" << endl
               << "id.version != sts.update_id_binding_version () ||" << endl
               << "imb.version == 0)"
               << "{"
               << "bind (imb.bind, id.bind, id.count, im, statement_update" <<
              (s.versioned ? ", svm" : "") << ");"
               << "sts.update_image_version (im.version);"
               << "sts.update_id_binding_version (id.version);"
               << "imb.version++;"
               << "}";

            os << "update_statement& st (sts.update_statement ());"
               << "if (";

            if (s.versioned)
              os << "!st.empty () && ";

            os << "st.execute () == 0)" << endl;

            if (opt == 0)
              os << "throw object_not_persistent ();";
            else
              os << "throw object_changed ();";

            os << endl;
          }

          // Update readwrite containers if any.
          //
          if (s.readwrite_containers)
          {
            instance<container_calls> t (container_calls::update_call, &s);
            t->traverse (c_);
          }

          // Update the optimistic concurrency version in the object member.
          // Very similar code to object.
          //
          if (s.optimistic ()) // Note: not update_opt.
          {
            member_access& ma_get (opt->get<member_access> ("get"));
            member_access& ma_set (opt->get<member_access> ("set"));

            // Object is passed as const reference so we need to cast away
            // constness.
            //
            string obj ("const_cast< object_type& > (obj)");
            string inc (optimistic_version_increment (*opt));

            if (!ma_set.synthesized)
              os << "// From " << location_string (ma_set.loc, true) << endl;

            if (ma_set.placeholder ())
            {
              if (!ma_get.synthesized)
                os << "// From " << location_string (ma_get.loc, true) << endl;

              if (inc == "1")
                os << ma_set.translate (
                  obj, ma_get.translate ("obj") + " + 1") << ";";
              else
                os << ma_set.translate (obj, inc) << ";";
            }
            else
            {
              // If this member is const and we have a synthesized direct
              // access, then cast away constness. Otherwise, we assume
              // that the user-provided expression handles this.
              //
              bool cast (ma_set.direct () && const_type (opt->type ()));
              if (cast)
                os << "const_cast< version_type& > (" << endl;

              os << ma_set.translate (obj);

              if (cast)
                os << ")";

              if (inc == "1")
                os << "++;";
              else
                os << " = " << inc << ";";
            }
          }

          os << "}";
        }

        section_extra (s);

        if (rs != 0)
          rs->base = 0;
      }

    protected:
      semantics::class_& c_;
      string scope_;
    };

    // Output a list of parameters for the persist statement.
    //
    struct persist_statement_params: object_columns_base, virtual context
    {
      typedef persist_statement_params base;

      persist_statement_params (string& params,
                                query_parameters& qp,
                                const string& sep)
          : params_ (params), qp_ (qp), sep_ (sep)
      {
      }

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_& c)
      {
        if (!inverse (m, key_prefix_))
          object_columns_base::traverse_pointer (m, c);
      }

      virtual bool
      traverse_column (semantics::data_member& m, string const&, bool first)
      {
        string p;

        if (version (m))
          p = version_value (m);
        else if (context::id (m) && auto_ (m)) // Only simple id can be auto.
          p = qp_.auto_id ();
        else
          p = qp_.next ();

        if (!p.empty ())
        {
          if (!first)
          {
            params_ += ',';
            params_ += sep_;
          }

          params_ += (p != "DEFAULT" ? convert_to (p, column_type (), m) : p);
        }

        return !p.empty ();
      }

      virtual string
      version_value (semantics::data_member&)
      {
        return "1";
      }

    private:
      string& params_;
      query_parameters& qp_;
      const string& sep_;
    };

    //
    //
    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      class_ ()
          : typedefs_ (false),
            query_columns_type_ (false, false, false),
            view_query_columns_type_ (false),
            grow_base_ (index_),
            grow_member_ (index_),
            grow_version_member_ (index_, "version_"),
            grow_discriminator_member_ (index_, "discriminator_"),
            bind_id_member_ ("id_"),
            bind_version_member_ ("version_"),
            bind_discriminator_member_ ("discriminator_"),
            init_id_image_member_ ("id_", "id"),
            init_version_image_member_ ("version_", "(*v)"),
            init_id_value_member_ ("id"),
            init_id_value_member_id_image_ ("id", "id_"),
            init_version_value_member_ ("v"),
            init_named_version_value_member_ ("v", "version_"),
            init_discriminator_value_member_ ("d", "", false),
            init_named_discriminator_value_member_ (
              "d", "discriminator_", false)
      {
        init ();
      }

      class_ (class_ const&)
          : root_context (), //@@ -Wextra
            context (),
            typedefs_ (false),
            query_columns_type_ (false, false, false),
            view_query_columns_type_ (false),
            grow_base_ (index_),
            grow_member_ (index_),
            grow_version_member_ (index_, "version_"),
            grow_discriminator_member_ (index_, "discriminator_"),
            bind_id_member_ ("id_"),
            bind_version_member_ ("version_"),
            bind_discriminator_member_ ("discriminator_"),
            init_id_image_member_ ("id_", "id"),
            init_version_image_member_ ("version_", "(*v)"),
            init_id_value_member_ ("id"),
            init_id_value_member_id_image_ ("id", "id_"),
            init_version_value_member_ ("v"),
            init_named_version_value_member_ ("v", "version_"),
            init_discriminator_value_member_ ("d", "", false),
            init_named_discriminator_value_member_ (
              "d", "discriminator_", false)
      {
        init ();
      }

      void
      init ()
      {
        *this >> defines_ >> *this;
        *this >> typedefs_ >> *this;

        if (generate_grow)
        {
          grow_base_inherits_ >> grow_base_;
          grow_member_names_ >> grow_member_;
        }

        bind_base_inherits_ >> bind_base_;
        bind_member_names_ >> bind_member_;

        init_image_base_inherits_ >> init_image_base_;
        init_image_member_names_ >> init_image_member_;

        init_value_base_inherits_ >> init_value_base_;
        init_value_member_names_ >> init_value_member_;
      }

      virtual void
      init_auto_id (semantics::data_member&, // id member
                    string const&)           // image variable prefix
      {
        if (insert_send_auto_id)
          assert (false);
      }

      virtual void
      init_image_pre (type&)
      {
      }

      virtual void
      init_value_extra ()
      {
      }

      virtual void
      traverse (type& c)
      {
        class_kind_type ck (class_kind (c));

        if (ck == class_other ||
            (!options.at_once () && class_file (c) != unit.file ()))
          return;

        names (c);

        context::top_object = context::cur_object = &c;

        switch (ck)
        {
        case class_object: traverse_object (c); break;
        case class_view: traverse_view (c); break;
        case class_composite: traverse_composite (c); break;
        default: break;
        }

        context::top_object = context::cur_object = 0;
      }

      //
      // statements
      //

      enum persist_position
      {
        persist_after_columns,
        persist_after_values
      };

      virtual string
      persist_statement_extra (type&, query_parameters&, persist_position)
      {
        return "";
      }

      virtual string
      update_statement_extra (type&)
      {
        return "";
      }

      //
      // common
      //

      virtual void
      post_query_ (type&, bool /*once_off*/)
      {
      }

      virtual void
      process_statement_columns (statement_columns&,
                                 statement_kind,
                                 bool /*dynamic*/)
      {
      }

      //
      // object
      //

      virtual void
      object_extra (type&) {}

      virtual void
      extra_statement_cache_extra_args (bool /*containers*/,
                                        bool /*sections*/) {}

      virtual void
      object_query_statement_ctor_args (type&,
                                        std::string const& q,
                                        bool process,
                                        bool /*prepared*/)
      {
        os << "conn," << endl
           << "text," << endl
           << process << "," << endl // Process.
           << "true," << endl        // Optimize.
           << q << ".parameters_binding ()," << endl
           << "imb";
      }

      virtual void
      object_erase_query_statement_ctor_args (type&)
      {
        os << "conn," << endl
           << "text," << endl
           << "q.parameters_binding ()";
      }

      virtual string
      optimistic_version_init (semantics::data_member&)
      {
        return "1";
      }

      // Returning "1" means increment by one.
      //
      virtual string
      optimistic_version_increment (semantics::data_member&)
      {
        return "1";
      }

      virtual bool
      optimistic_insert_bind_version (semantics::data_member&)
      {
        return false;
      }

      virtual void
      traverse_object (type& c);

      //
      // view
      //

      virtual void
      view_extra (type&)
      {
      }

      virtual void
      view_query_statement_ctor_args (type&,
                                      string const& q,
                                      bool process,
                                      bool /*prepared*/)
      {
        os << "conn," << endl
           << q << ".clause ()," << endl
           << process << "," << endl   // Process.
           << "true," << endl          // Optimize.
           << q << ".parameters_binding ()," << endl
           << "imb";
      }

      virtual void
      traverse_view (type& c);

      struct expression
      {
        explicit
        expression (std::string const& v): kind (literal), value (v) {}
        expression (view_object* vo): kind (pointer), vo (vo) {}

        enum kind_type {literal, pointer};

        kind_type kind;
        std::string value;
        data_member_path member_path;
        view_object* vo;
      };

      expression
      translate_expression (type& c,
                            cxx_tokens const&,
                            semantics::scope& start_scope,
                            location_t loc,
                            string const& prag,
                            bool* placeholder = 0,
                            bool predicate = true);
      //
      // composite
      //

      virtual void
      traverse_composite (type& c)
      {
        bool versioned (context::versioned (c));

        string const& type (class_fq_name (c));
        string traits ("access::composite_value_traits< " + type + ", id_" +
                       db.string () + " >");

        os << "// " << class_name (c) << endl
           << "//" << endl
           << endl;

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
          inherits (c, grow_base_inherits_);
          names (c, grow_member_names_);

          os << "return grew;"
             << "}";
        }

        // bind (image_type)
        //
        os << "void " << traits << "::" << endl
           << "bind (" << bind_vector << " b," << endl
           << "image_type& i," << endl
           << db << "::statement_kind sk";

        if (versioned)
          os << "," << endl
             << "const schema_version_migration& svm";

        os << ")"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (b);"
           << "ODB_POTENTIALLY_UNUSED (i);"
           << "ODB_POTENTIALLY_UNUSED (sk);";

        if (versioned)
          os << "ODB_POTENTIALLY_UNUSED (svm);";

        os << endl
           << "using namespace " << db << ";"
           << endl;

        if (readonly (c))
          os << "assert (sk != statement_update);"
             << endl;

        os << "std::size_t n (0);"
           << "ODB_POTENTIALLY_UNUSED (n);"
           << endl;

        inherits (c, bind_base_inherits_);
        names (c, bind_member_names_);

        os << "}";

        // init (image, value)
        //
        os << (generate_grow ? "bool " : "void ") << traits << "::" << endl
           << "init (image_type& i," << endl
           << "const value_type& o," << endl
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

        if (readonly (c))
          os << "assert (sk != statement_update);"
             << endl;

        if (generate_grow)
          os << "bool grew (false);"
             << endl;

        inherits (c, init_image_base_inherits_);
        names (c, init_image_member_names_);

        if (generate_grow)
          os << "return grew;";

        os << "}";

        // init (value, image)
        //
        os << "void " << traits << "::" << endl
           << "init (value_type& o," << endl
           << "const image_type&  i," << endl
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

        inherits (c, init_value_base_inherits_);
        names (c, init_value_member_names_);

        os << "}";
      }

    private:
      traversal::defines defines_;
      typedefs typedefs_;

      instance<query_columns_type> query_columns_type_;
      instance<view_query_columns_type> view_query_columns_type_;

      size_t index_;
      instance<grow_base> grow_base_;
      traversal::inherits grow_base_inherits_;
      instance<grow_member> grow_member_;
      traversal::names grow_member_names_;
      instance<grow_member> grow_version_member_;
      instance<grow_member> grow_discriminator_member_;


      instance<bind_base> bind_base_;
      traversal::inherits bind_base_inherits_;
      instance<bind_member> bind_member_;
      traversal::names bind_member_names_;
      instance<bind_member> bind_id_member_;
      instance<bind_member> bind_version_member_;
      instance<bind_member> bind_discriminator_member_;

      instance<init_image_base> init_image_base_;
      traversal::inherits init_image_base_inherits_;
      instance<init_image_member> init_image_member_;
      traversal::names init_image_member_names_;

      instance<init_image_member> init_id_image_member_;
      instance<init_image_member> init_version_image_member_;

      instance<init_value_base> init_value_base_;
      traversal::inherits init_value_base_inherits_;
      instance<init_value_member> init_value_member_;
      traversal::names init_value_member_names_;

      instance<init_value_member> init_id_value_member_;
      instance<init_value_member> init_id_value_member_id_image_;
      instance<init_value_member> init_version_value_member_;
      instance<init_value_member> init_named_version_value_member_;
      instance<init_value_member> init_discriminator_value_member_;
      instance<init_value_member> init_named_discriminator_value_member_;
    };

    struct include: virtual context
    {
      typedef include base;

      virtual void
      generate ()
      {
        extra_pre ();

        os << "#include <cassert>" << endl
           << "#include <cstring>  // std::memcpy" << endl;

        if (features.polymorphic_object)
          os << "#include <typeinfo>" << endl;

        os << endl;

        if (features.polymorphic_object)
          os << "#include <odb/polymorphic-map.hxx>" << endl;

        if (embedded_schema)
          os << "#include <odb/schema-catalog-impl.hxx>" << endl;

        if (multi_dynamic)
          os << "#include <odb/function-table.hxx>" << endl;

        os << endl;

        os << "#include <odb/" << db << "/traits.hxx>" << endl
           << "#include <odb/" << db << "/database.hxx>" << endl
           << "#include <odb/" << db << "/transaction.hxx>" << endl
           << "#include <odb/" << db << "/connection.hxx>" << endl
           << "#include <odb/" << db << "/statement.hxx>" << endl
           << "#include <odb/" << db << "/statement-cache.hxx>" << endl;

        if (features.simple_object)
          os << "#include <odb/" << db << "/simple-object-statements.hxx>" << endl;

        if (features.polymorphic_object)
          os << "#include <odb/" << db << "/polymorphic-object-statements.hxx>" << endl;

        if (features.no_id_object)
          os << "#include <odb/" << db << "/no-id-object-statements.hxx>" << endl;

        if (features.view)
          os << "#include <odb/" << db << "/view-statements.hxx>" << endl;

        if (features.section)
          os << "#include <odb/" << db << "/section-statements.hxx>" << endl;

        os << "#include <odb/" << db << "/container-statements.hxx>" << endl
           << "#include <odb/" << db << "/exceptions.hxx>" << endl;

        if (options.generate_query ())
        {
          if (options.generate_prepared ())
            os << "#include <odb/" << db << "/prepared-query.hxx>" << endl;

          if (features.simple_object)
            os << "#include <odb/" << db << "/simple-object-result.hxx>" << endl;

          if (features.polymorphic_object)
            os << "#include <odb/" << db << "/polymorphic-object-result.hxx>" << endl;

          if (features.no_id_object)
            os << "#include <odb/" << db << "/no-id-object-result.hxx>" << endl;

          if (features.view)
            os << "#include <odb/" << db << "/view-result.hxx>" << endl;
        }

        extra_post ();

        os << endl;
      }

      virtual void
      extra_pre ()
      {
      }

      virtual void
      extra_post ()
      {
      }
    };
  }
}

#endif // ODB_RELATIONAL_SOURCE_HXX
