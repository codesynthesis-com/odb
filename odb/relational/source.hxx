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
                      query_parameters* param = 0)
          : object_columns_base (true, true),
            sk_ (sk), sc_ (sc), param_ (param), depth_ (1)
      {
      }

      object_columns (std::string const& table_qname,
                      statement_kind sk,
                      statement_columns& sc,
                      size_t depth = 1)
          : object_columns_base (true, true),
            sk_ (sk),
            sc_ (sc),
            param_ (0),
            table_name_ (table_qname),
            depth_ (depth)
      {
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

        // Ignore certain columns depending on what kind statement we are
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
            sk_ == statement_update)
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
                                size_t depth,
                                string const& alias = "",
                                string const prefix = "",
                                string const& suffix = "\n")
          : object_columns_base (true, true),
            obj_ (obj),
            depth_ (depth),
            alias_ (alias),
            prefix_ (prefix),
            suffix_ (suffix)
      {
        // Get the table and id columns.
        //
        table_ = alias_.empty ()
          ? table_qname (obj)
          : quote_id (alias_ + "_" + table_name (obj).uname ());

        cols_->traverse (*id_member (obj));
      }

      virtual void
      traverse_object (semantics::class_& c)
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

        string line (" LEFT JOIN " + quote_id (table));

        if (!alias_.empty ())
          line += (need_alias_as ? " AS " : " ") + alias;

        line += " ON " + cond.str ();

        os << prefix_ << strlit (line) << suffix_;

        if (--depth_ != 0)
          inherits (c);
      }

    private:
      semantics::class_& obj_;
      size_t depth_;
      string alias_;
      string prefix_;
      string suffix_;
      string table_;
      instance<object_columns_list> cols_;
    };

    struct object_joins: object_columns_base, virtual context
    {
      typedef object_joins base;

      //@@ context::{cur,top}_object; might have to be created every time.
      //
      object_joins (semantics::class_& scope, bool query, size_t depth = 1)
          : object_columns_base (true, true),
            query_ (query),
            depth_ (depth),
            table_ (table_qname (scope)),
            id_ (*id_member (scope))
      {
        id_cols_->traverse (id_);
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
          string line (" LEFT JOIN ");
          line += t;

          if (!a.empty ())
            line += (need_alias_as ? " AS " : " ") + a;

          line += " ON ";
          line += cond.str ();

          os << strlit (line) << endl;
        }

        // Add dependent join (i.e., an object table join via the
        // container table).
        //
        if (!dt.empty ())
        {
          string line (" LEFT JOIN ");
          line += dt;

          if (!da.empty ())
            line += (need_alias_as ? " AS " : " ") + da;

          line += " ON ";
          line += dcond.str ();

          os << strlit (line) << endl;
        }

        // If we joined the object that is part of a polymorphic type
        // hierarchy, then we may need join its bases as well as its
        // derived types.
        //
        if (joined_obj != 0 && poly)
        {
          size_t depth (polymorphic_depth (*joined_obj));

          // Join "up" (derived).
          //
          if (joined_obj != &c)
          {
            size_t d (polymorphic_depth (c) - depth); //@@ (im)perfect forward.
            instance<polymorphic_object_joins> t (*joined_obj, d, alias);
            t->traverse (c);
          }

          // Join "down" (base).
          //
          if (joined_obj != poly_root)
          {
            size_t d (depth - 1); //@@ (im)perfect forward.
            instance<polymorphic_object_joins> t (*joined_obj, d, alias);
            t->traverse (polymorphic_base (*joined_obj));
          }
        }
      }

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

      bind_member (string const& var = string (),
                   string const& arg = string ())
          : member_base (var, 0, string (), string ()),
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
          os << "// " << mi.m.name () << endl
             << "//" << endl;

          if (!insert_send_auto_id && id (mi.m) && auto_ (mi.m))
            os << "if (sk != statement_insert && sk != statement_update)"
               << "{";
          else if (inverse (mi.m, key_prefix_) || version (mi.m))
            os << "if (sk == statement_select)"
               << "{";
          // If the whole class is readonly, then we will never be
          // called with sk == statement_update.
          //
          else if (!readonly (*context::top_object))
          {
            semantics::class_* c;

            if (id (mi.m) ||
                readonly (mi.m) ||
                ((c = composite (mi.t)) && readonly (*c)))
              os << "if (sk != statement_update)"
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
          semantics::class_* c;

          if ((c = composite (mi.t)))
          {
            bool ro (readonly (*c));
            column_count_type const& cc (column_count (*c));

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
          else if (inverse (mi.m, key_prefix_) || version (mi.m))
            block = true;
          else if (!readonly (*context::top_object))
          {
            semantics::class_* c;

            if (id (mi.m) ||
                readonly (mi.m) ||
                ((c = composite (mi.t)) && readonly (*c)))
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
           << "b + n, " << arg << "." << mi.var << "value, sk);";
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

        os << class_fq_name (c) << ", id_" << db << " >::bind (b + n, i, sk);";

        column_count_type const& cc (column_count (c));

        os << "n += " << cc.total << "UL";

        // select = total
        // insert = total - inverse - optimistic_managed
        // update = total - inverse - optimistic_managed - id - readonly
        //
        if (cc.inverse != 0 ||
            cc.optimistic_managed != 0 ||
            (!ro && (cc.id != 0 || cc.readonly != 0)))
        {
          os << " - (" << endl
             << "sk == statement_select ? 0 : ";

          if (cc.inverse != 0 || cc.optimistic_managed != 0)
            os << (cc.inverse + cc.optimistic_managed) << "UL";

          if (!ro && (cc.id != 0 || cc.readonly != 0))
          {
            if (cc.inverse != 0 || cc.optimistic_managed != 0)
              os << " + ";

            os << "(" << endl
               << "sk == statement_insert ? ";

            if (insert_send_auto_id || !auto_ (*id_member (c)))
              os << "0";
            else
              os << cc.id << "UL";

            os << " : " << cc.id + cc.readonly << "UL)";
          }

          os << ")";
        }

        os << ";";

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

      grow_member (size_t& index, string const& var = string ())
          : member_base (var, 0, string (), string ()), index_ (index)
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
           << "i, t + " << index_ << "UL))" << endl
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
                         string const& member = string ())
          : member_base (var, 0, string (), string ()),
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

        // Ignore polymorphic id references; they are initialized in a
        // special way.
        //
        if (mi.ptr != 0 && mi.m.count ("polymorphic-ref"))
          return false;

        bool comp (composite (mi.t));

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

          // If the whole class is readonly, then we will never be
          // called with sk == statement_update.
          //
          if (!readonly (*context::top_object))
          {
            semantics::class_* c;

            if (id (mi.m) ||
                readonly (mi.m) ||
                ((c = composite (mi.t)) && readonly (*c))) // Can't be id.
              os << "if (sk == statement_insert)";
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
            if (!comp)
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
        if (mi.wrapper != 0 && comp)
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
               << "i." << mi.var << "value, sk);"
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
        else if (comp)
          type = mi.fq_type ();
        else
        {
          type = mi.fq_type ();

          // Indicate to the value_traits whether this column can be NULL.
          //
          os << "bool is_null (" <<
            (null (mi.m, key_prefix_) ? "true" : "false") << ");";
        }

        if (comp)
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
          os << "}"
             << "else" << endl;

          if (!null (mi.m, key_prefix_))
            os << "throw null_pointer ();";
          else if (composite (mi.t))
            os << traits << "::set_null (i." << mi.var << "value, sk);";
          else
            set_null (mi);
        }

        if (mi.wrapper != 0 && composite (mi.t))
        {
          if (null (mi.m, key_prefix_) &&
              mi.wrapper->template get<bool> ("wrapper-null-handler"))
            os << "}";
        }

        os << "}";
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
           << "sk)";

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

        os << class_fq_name (c) << ", id_" << db << " >::init (i, o, sk)";

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
                         bool ignore_implicit_discriminator = true)
          : member_base (var, 0, string (), string ()),
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

        // Ignore polymorphic id references; they are initialized in a
        // special way.
        //
        if (mi.ptr != 0 && mi.m.count ("polymorphic-ref"))
          return false;

        // Ignore implicit discriminators.
        //
        if (ignore_implicit_discriminator_ && discriminator (mi.m))
          return false;

        bool comp (composite (mi.t));

        if (!member_override_.empty ())
        {
          os << "{";
          member = member_override_;
        }
        else
        {
          os << "// " << mi.m.name () << endl
             << "//" << endl
             << "{";

          // Get the member using the accessor expression.
          //
          member_access& ma (mi.m.template get<member_access> ("set"));

          // Make sure this kind of member can be modified with this
          // kind of accessor (database-specific, e.g., streaming).
          //
          if (!comp)
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
        if (mi.wrapper != 0 && comp)
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
               << "i." << mi.var << "value))" << endl
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

          if (comp)
            os << "composite_value_traits< " << type << ", id_" << db <<
              " >::get_null (" << endl
               << "i." << mi.var << "value)";
          else
            get_null (mi);

          os << ")" << endl;

          if (!null (mi.m, key_prefix_) )
            os << "throw null_pointer ();";
          else
            os << member << " = ptr_traits::pointer_type ();";

          os << "else"
             << "{";

          os << type << " id;";

          member = "id";
        }
        else
          type = mi.fq_type ();

        if (comp)
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
           << "db);"
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

        os << class_fq_name (c) << ", id_" << db << " >::init (o, i, db);"
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
      process_statement_columns (statement_columns&, statement_kind)
      {
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

            process_statement_columns (sc, statement_select);

            os << strlit ("SELECT ") << endl;

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << strlit (c + (++i != e ? "," : "")) << endl;
            }

            instance<query_parameters> qp (inv_table);
            os << strlit (" FROM " + inv_qtable);

            for (object_columns_list::iterator b (inv_fid_cols->begin ()),
                   i (b); i != inv_fid_cols->end (); ++i)
            {
              os << endl
                 << strlit ((i == b ? " WHERE " : " AND ") +
                            inv_qtable + "." + quote_id (i->name) + "=" +
                            convert_to (qp->next (), i->type, *i->member));
            }
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

            process_statement_columns (sc, statement_select);

            os << strlit ("SELECT ") << endl;

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << strlit (c + (++i != e ? "," : "")) << endl;
            }

            instance<query_parameters> qp (table);
            os << strlit (" FROM " + qtable);

            for (object_columns_list::iterator b (id_cols->begin ()), i (b);
                 i != id_cols->end (); ++i)
            {
              os << endl
                 << strlit ((i == b ? " WHERE " : " AND ") +
                            qtable + "." + quote_id (i->name) + "=" +
                            convert_to (qp->next (), i->type, *i->member));
            }

            if (ordered)
            {
              // Top-level column.
              //
              string const& col (
                column_qname (m, "index", "index", column_prefix ()));

              os << endl
                 << strlit (" ORDER BY " + qtable + "." + col);
            }
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

            process_statement_columns (sc, statement_insert);

            os << strlit ("INSERT INTO " + qtable + " (") << endl;

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << strlit (c + (++i != e ? "," : ")")) << endl;
            }

            string values;
            instance<query_parameters> qp (table);
            for (statement_columns::const_iterator b (sc.begin ()), i (b),
                   e (sc.end ()); i != e; ++i)
            {
              if (i != b)
                values += ',';

              values += convert_to (qp->next (), i->type, *i->member);
            }

            os << strlit (" VALUES (" + values + ")") << ";"
               << endl;
          }

          // update_statement
          //
          if (smart)
          {
            os << "const char " << scope << "::" << endl
               << "update_statement[] =" << endl
               << strlit ("UPDATE " + qtable + " SET ");

            instance<query_parameters> qp (table);
            statement_columns sc;
            {
              query_parameters* p (qp.get ()); // Imperfect forwarding.
              statement_kind sk (statement_update); // Imperfect forwarding.
              instance<object_columns> t (sk, sc, p);
              t->traverse (m, vt, "value", "value");
              process_statement_columns (sc, statement_update);
            }

            for (statement_columns::const_iterator i (sc.begin ()),
                   e (sc.end ()); i != e;)
            {
              string const& c (i->column);
              os << endl
                 << strlit (c + (++i != e ? "," : ""));
            }

            for (object_columns_list::iterator b (id_cols->begin ()), i (b);
                 i != id_cols->end (); ++i)
            {
              os << endl
                 << strlit ((i == b ? " WHERE " : " AND ") +
                            quote_id (i->name) + "=" +
                            convert_to (qp->next (), i->type, *i->member));
            }

            for (object_columns_list::iterator b (ik_cols->begin ()), i (b);
                 i != ik_cols->end (); ++i)
            {
              os << endl
                 << strlit (" AND " + quote_id (i->name) + "=" +
                            convert_to (qp->next (), i->type, *i->member));
            }

            os << ";"
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

            os << strlit ("DELETE FROM " + qtable);

            for (object_columns_list::iterator b (id_cols->begin ()), i (b);
                 i != id_cols->end (); ++i)
            {
              os << endl
                 << strlit ((i == b ? " WHERE " : " AND ") +
                            quote_id (i->name) + "=" +
                            convert_to (qp->next (), i->type, *i->member));
            }

            if (smart)
            {
              for (object_columns_list::iterator b (ik_cols->begin ()), i (b);
                   i != ik_cols->end (); ++i)
              {
                os << endl
                   << strlit (" AND " + quote_id (i->name) +
                              (ck == ck_ordered ? ">=" : "=") +
                              convert_to (qp->next (), i->type, *i->member));
              }
            }

            os << ";"
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
             << "n += id_size;"
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
             << "data_image_type& d)"
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
             << "n += id_size;"
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
             << "data_image_type& d)"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "statement_kind sk (statement_update);"
             << "ODB_POTENTIALLY_UNUSED (sk);"
             << endl
             << "std::size_t n (0);"
             << endl;

          os << "// key" << endl
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
             << "n += id_size;"
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
             << "grow (data_image_type& i, " << truncated_vector << " t)"
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
          os << "void " << scope << "::" << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              if (ordered)
                os << "init (data_image_type& i, index_type* j, " <<
                  "const value_type& v)";
              else
                os << "init (data_image_type& i, const value_type& v)";
              break;
            }
          case ck_map:
          case ck_multimap:
            {
              os << "init (data_image_type& i, const key_type* k, " <<
                "const value_type& v)";
              break;
            }
          case ck_set:
          case ck_multiset:
            {
              os << "init (data_image_type& i, const value_type& v)";
              break;
            }
          }

          os << "{"
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
        os << "void " << scope << "::" << endl;

        switch (ck)
        {
        case ck_ordered:
          {
            if (ordered)
              os << "init (index_type& j, value_type& v, " <<
                "const data_image_type& i, database* db)";
            else
              os << "init (value_type& v, const data_image_type& i, " <<
                "database* db)";

            os << "{"
               << "ODB_POTENTIALLY_UNUSED (db);"
               << endl;

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
            os << "init (key_type& k, value_type& v, " <<
              "const data_image_type& i, database* db)"
               << "{"
               << "ODB_POTENTIALLY_UNUSED (db);"
               << endl
               << "// key" << endl
               << "//" << endl;

            instance<init_value_member> im (
              "key_", "k", *kt, "key_type", "key");
            im->traverse (m);

            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "init (value_type& v, const data_image_type& i, " <<
              "database* db)"
               << "{"
               << "ODB_POTENTIALLY_UNUSED (db);"
               << endl;
            break;
          }
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
               << "data_image_type& di (sts.data_image ());"
               << endl;

            switch (ck)
            {
            case ck_ordered:
              {
                os << "init (di, " << (ordered ? "&i, " : "") << "v);";
                break;
              }
            case ck_map:
            case ck_multimap:
              {
                os << "init (di, &k, v);";
                break;
              }
            case ck_set:
            case ck_multiset:
              {
                os << "init (di, v);";
                break;
              }
            }

            os << endl
               << "if (sts.data_binding_test_version ())"
               << "{"
               << "const binding& id (sts.id_binding ());"
               << "bind (sts.data_bind (), id.bind, id.count, di);"
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
             << "data_image_type& di (sts.data_image ());"
             << endl;

          switch (ck)
          {
          case ck_ordered:
            {
              os << "init (ci, i);";
              os << "init (di, 0, v);";
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
             << "bind (sts.update_bind (), id.bind, id.count, ci, di);"
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

        // Extract current element.
        //
        switch (ck)
        {
        case ck_ordered:
          {
            os << "init (" << (ordered ? "i, " : "") <<
              "v, di, &sts.connection ().database ());"
               << endl;
            break;
          }
        case ck_map:
        case ck_multimap:
          {
            os << "init (k, v, di, &sts.connection ().database ());"
               << endl;
            break;
          }
        case ck_set:
        case ck_multiset:
          {
            os << "init (v, di, &sts.connection ().database ());"
               << endl;
            break;
          }
        }

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
             << "bind (sts.data_bind (), id.bind, id.count, di);"
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
             << "grow (di, sts.select_image_truncated ());"
             << endl
             << "if (sts.data_binding_test_version ())"
             << "{"
            // Id cannot change.
            //
             << "bind (sts.data_bind (), 0, sts.id_binding ().count, di);"
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
             << "persist (const container_type& c, statements_type& sts)"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "functions_type& fs (sts.functions ());";

          if (!smart && ck == ck_ordered)
            os << "fs.ordered_ = " << (ordered ? "true" : "false") << ";";

          os << "container_traits_type::persist (c, fs);"
             << "}";
        }

        // load
        //
        os << "void " << scope << "::" << endl
           << "load (container_type& c, statements_type& sts)"
           << "{"
           << "using namespace " << db << ";"
           << "using " << db << "::select_statement;" // Conflicts.
           << endl
           << "const binding& id (sts.id_binding ());"
           << endl
           << "if (sts.data_binding_test_version ())"
           << "{"
           << "bind (sts.data_bind (), id.bind, id.count, sts.data_image ());"
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
             << "grow (di, sts.select_image_truncated ());"
             << endl
             << "if (sts.data_binding_test_version ())"
             << "{"
            // Id cannot change.
            //
             << "bind (sts.data_bind (), 0, id.count, di);"
             << "sts.data_binding_update_version ();"
             << "st.refetch ();"
             << "}"
             << "}";

        os << "bool more (r != select_statement::no_data);"
           << endl
           << "functions_type& fs (sts.functions ());";

        if (!smart && ck == ck_ordered)
          os << "fs.ordered_ = " << (ordered ? "true" : "false") << ";";

        os << "container_traits_type::load (c, more, fs);"
           << "}";

        // update
        //
        if (!(inverse || readonly (member_path_, member_scope_)))
        {
          os << "void " << scope << "::" << endl
             << "update (const container_type& c, statements_type& sts)"
             << "{"
             << "using namespace " << db << ";"
             << endl
             << "const binding& id (sts.id_binding ());"
             << endl
             << "if (sts.data_binding_test_version ())"
             << "{"
             << "bind (sts.data_bind (), id.bind, id.count, sts.data_image ());"
             << "sts.data_binding_update_version ();"
             << "}"
             << "functions_type& fs (sts.functions ());";

          if (!smart && ck == ck_ordered)
            os << "fs.ordered_ = " << (ordered ? "true" : "false") << ";";

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
            os << "fs.ordered_ = " << (ordered ? "true" : "false") << ";";

          os << "container_traits_type::erase (" << (smart ? "c, " : "") << "fs);"
             << "}";
        }
      }

    protected:
      string scope_;
      semantics::class_& c_;
    };

    // Container statement cache members.
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
        erase_id_call
      };

      container_calls (call_type call)
          : object_members_base (true, false, true),
            call_ (call),
            obj_prefix_ ("obj"),
            modifier_ (0)
      {
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

        // In certain cases we don't need to do anything.
        //
        if ((call_ != load_call && inverse) ||
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
               << "sts.container_statment_cache ()." << sts_name << ");";
            break;
          }
        case load_call:
          {
            os << traits << "::load (" << endl
               << var << "," << endl
               << "sts.container_statment_cache ()." << sts_name << ");";
            break;
          }
        case update_call:
          {
            os << traits << "::update (" << endl
               << var << "," << endl
               << "sts.container_statment_cache ()." << sts_name << ");";
            break;
          }
        case erase_obj_call:
          {
            os << traits << "::erase (" << endl;

            if (smart)
              os << "&" << var << "," << endl;

            os << "sts.container_statment_cache ()." << sts_name << ");"
               << endl;
            break;
          }
        case erase_id_call:
          {
            os << traits << "::erase (" << endl;

            if (smart)
              os << "0," << endl;

            os << "sts.container_statment_cache ()." << sts_name << ");"
               << endl;
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

    // Output a list of parameters for the persist statement.
    //
    struct persist_statement_params: object_columns_base, virtual context
    {
      typedef persist_statement_params base;

      persist_statement_params (string& params, query_parameters& qp)
          : params_ (params), qp_ (qp)
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
            params_ += ',';

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
    };

    //
    //
    struct class_: traversal::class_, virtual context
    {
      typedef class_ base;

      class_ ()
          : query_columns_type_ (false, false, false),
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
        if (!options.at_once () && class_file (c) != unit.file ())
          return;

        context::top_object = context::cur_object = &c;

        if (object (c))
          traverse_object (c);
        else if (view (c))
          traverse_view (c);
        else if (composite (c))
          traverse_composite (c);

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

      virtual void
      persist_statement_extra (type&, query_parameters&, persist_position)
      {
      }

      virtual void
      update_statement_extra (type&)
      {
      }

      //
      // common
      //

      virtual void
      post_query_ (type&, bool /*once_off*/)
      {
      }

      virtual void
      process_statement_columns (statement_columns&, statement_kind)
      {
      }

      //
      // object
      //

      virtual void
      object_extra (type&) {}

      virtual void
      container_cache_extra_args (bool /*used*/) {}

      virtual void
      object_query_statement_ctor_args (type&,
                                        std::string const& q,
                                        bool /*prepared*/)
      {
        os << "conn," << endl
           << "query_statement + " << q << ".clause ()," << endl
           << q << ".parameters_binding ()," << endl
           << "imb";
      }

      virtual void
      object_erase_query_statement_ctor_args (type&)
      {
        os << "conn," << endl
           << "erase_query_statement + q.clause ()," << endl
           << "q.parameters_binding ()";
      }

      virtual string
      optimimistic_version_init (semantics::data_member&)
      {
        return "1";
      }

      // Returning "1" means incremenet by one.
      //
      virtual string
      optimimistic_version_increment (semantics::data_member&)
      {
        return "1";
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
                                      bool /*prepared*/)
      {
        os << "conn," << endl
           << q << ".clause ()," << endl
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
             << "grow (image_type& i, " << truncated_vector << " t)"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (i);"
             << "ODB_POTENTIALLY_UNUSED (t);"
             << endl
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
           << "bind (" << bind_vector << " b, image_type& i, " <<
          db << "::statement_kind sk)"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (b);"
           << "ODB_POTENTIALLY_UNUSED (i);"
           << "ODB_POTENTIALLY_UNUSED (sk);"
           << endl
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
           << "init (image_type& i, const value_type& o, " <<
          db << "::statement_kind sk)"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (i);"
           << "ODB_POTENTIALLY_UNUSED (o);"
           << "ODB_POTENTIALLY_UNUSED (sk);"
           << endl
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
           << "init (value_type& o, const image_type&  i, database* db)"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (o);"
           << "ODB_POTENTIALLY_UNUSED (i);"
           << "ODB_POTENTIALLY_UNUSED (db);"
           << endl;

        inherits (c, init_value_base_inherits_);
        names (c, init_value_member_names_);

        os << "}";
      }

    private:
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
      instance<init_value_member> init_version_value_member_;
      instance<init_value_member> init_named_version_value_member_;
      instance<init_value_member> init_discriminator_value_member_;
      instance<init_value_member> init_named_discriminator_value_member_;

      instance<schema::cxx_object> schema_;
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
