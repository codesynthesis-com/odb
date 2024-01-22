// file      : odb/relational/schema.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_SCHEMA_HXX
#define ODB_RELATIONAL_SCHEMA_HXX

#include <set>
#include <vector>
#include <cassert>

#include <odb/emitter.hxx>
#include <odb/relational/common.hxx>
#include <odb/relational/context.hxx>

namespace relational
{
  namespace schema
  {
    typedef std::set<qname> table_set;

    struct common: virtual context
    {
      typedef ::emitter emitter_type;

      common (emitter_type& e, ostream& os, schema_format f)
          : e_ (e), os_ (os), format_ (f) {}

      void
      pre_statement ()
      {
        e_.pre ();
        diverge (os_);
      }

      void
      post_statement ()
      {
        restore ();
        e_.post ();
      }

      emitter_type&
      emitter () const
      {
        return e_;
      }

      ostream&
      stream () const
      {
        return os_;
      }

    public:
      // Find an entity corresponding to the drop node in alter_table.
      //
      template <typename T, typename D>
      T&
      find (D& d)
      {
        using sema_rel::model;
        using sema_rel::changeset;
        using sema_rel::table;
        using sema_rel::alter_table;

        alter_table& at (dynamic_cast<alter_table&> (d.scope ()));
        changeset& cs (dynamic_cast<changeset&> (at.scope ()));
        model& bm (cs.base_model ());
        table* bt (bm.find<table> (at.name ()));
        assert (bt != 0);
        T* b (bt->find<T> (d.name ()));
        assert (b != 0);
        return *b;
      }

    protected:
      emitter_type& e_;
      ostream& os_;
      schema_format format_;
    };

    //
    // Drop.
    //

    // Only used in migration.
    //
    struct drop_column: trav_rel::drop_column, common
    {
      typedef drop_column base;

      drop_column (common const& c, bool* first = 0)
          : common (c),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true)
      {
      }

      drop_column (drop_column const& c)
          : root_context (), // @@ -Wextra
            context (),
            common (c),
            first_ (&c.first_ != &c.first_data_ ? c.first_ : first_data_),
            first_data_ (c.first_data_)
      {
      }

      virtual void
      drop_header ()
      {
        // By default ADD COLUMN though some databases use just ADD.
        //
        os << "DROP COLUMN ";
      }

      virtual void
      traverse (sema_rel::drop_column& dc)
      {
        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl
           << "  ";
        drop_header ();
        os << quote_id (dc.name ());
      }

    protected:
      bool& first_;
      bool first_data_;
    };

    // Normally only used in migration but some databases use it as a
    // base to also drop foreign keys in schema.
    //
    struct drop_foreign_key: trav_rel::foreign_key,
                             trav_rel::drop_foreign_key,
                             trav_rel::add_foreign_key, // Override.
                             common
    {
      typedef drop_foreign_key base;

      // Schema constructor.
      //
      drop_foreign_key (common const& c, table_set& dropped, bool* first = 0)
          : common (c),
            dropped_ (&dropped),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true)
      {
      }

      // Migration constructor.
      //
      drop_foreign_key (common const& c, bool* first = 0)
          : common (c),
            dropped_ (0),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true)
      {
      }

      drop_foreign_key (drop_foreign_key const& c)
          : root_context (), // @@ -Wextra
            context (),
            common (c),
            dropped_ (c.dropped_),
            first_ (&c.first_ != &c.first_data_ ? c.first_ : first_data_),
            first_data_ (c.first_data_)
      {
      }

      virtual void
      drop_header ()
      {
        os << "DROP CONSTRAINT ";
      }

      virtual void
      traverse (sema_rel::foreign_key& fk)
      {
        // If the table which we reference is droped before us, then
        // we need to drop the constraint first. Similarly, if the
        // referenced table is not part if this model, then assume
        // it is dropped before us. In migration we always do this
        // first.
        //
        sema_rel::table& t (dynamic_cast<sema_rel::table&> (fk.scope ()));

        if (dropped_ != 0)
        {
          sema_rel::qname const& rt (fk.referenced_table ());
          sema_rel::model& m (dynamic_cast<sema_rel::model&> (t.scope ()));

          if (dropped_->find (rt) == dropped_->end () &&
              m.find (rt) != m.names_end ())
            return;
        }

        drop (t, fk);
      }

      virtual void
      drop (sema_rel::table& t, sema_rel::foreign_key& fk)
      {
        // When generating schema we would need to check if the key exists.
        // So this implementation will need to be customize on the per-
        // database level.
        //
        pre_statement ();

        os << "ALTER TABLE " << quote_id (t.name ()) << endl
           << "  ";
        drop_header ();
        os << quote_id (fk.name ()) << endl;

        post_statement ();
      }

      virtual void
      traverse (sema_rel::drop_foreign_key& dfk)
      {
        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl;
        drop (dfk);
      }

      virtual void
      drop (sema_rel::drop_foreign_key& dfk)
      {
        os << "  ";
        drop_header ();
        os << quote_id (dfk.name ());
      }

    protected:
      table_set* dropped_;
      bool& first_;
      bool first_data_;
    };

    // Currently only used in migration.
    //
    struct drop_index: trav_rel::drop_index, common
    {
      typedef drop_index base;

      enum index_type {unique, non_unique, all};

      drop_index (common const& c, index_type t = all)
          : common (c), type_ (t) {}

      virtual void
      traverse (sema_rel::drop_index& di)
      {
        // Find the index we are dropping in the base model.
        //
        traverse (find<sema_rel::index> (di));
      }

      virtual void
      traverse (sema_rel::index& in)
      {
        if (type_ == unique &&
            in.type ().find ("UNIQUE") == string::npos &&
            in.type ().find ("unique") == string::npos)
          return;

        if (type_ == non_unique && (
              in.type ().find ("UNIQUE") != string::npos ||
              in.type ().find ("unique") != string::npos))
          return;

        pre_statement ();
        drop (in);
        post_statement ();
      }

      virtual string
      name (sema_rel::index& in)
      {
        return quote_id (in.name ());
      }

      virtual void
      drop (sema_rel::index& in)
      {
        os << "DROP INDEX " << name (in) << endl;
      }

    protected:
      index_type type_;
    };

    struct drop_table: trav_rel::table,
                       trav_rel::drop_table,
                       trav_rel::add_table,   // Override.
                       trav_rel::alter_table, // Override.
                       common
    {
      typedef drop_table base;

      drop_table (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f) {}

      virtual void
      drop (sema_rel::table& t, bool migration)
      {
        pre_statement ();
        os << "DROP TABLE " << (migration ? "" : "IF EXISTS ") <<
          quote_id (t.name ()) << endl;
        post_statement ();
      }

      virtual void
      delete_ (sema_rel::qname const& rtable,
               sema_rel::qname const& dtable,
               sema_rel::primary_key& rkey,
               sema_rel::primary_key& dkey)
      {
        pre_statement ();

        // This might not be the most efficient way for every database.
        //
        os << "DELETE FROM " << quote_id (rtable) << endl
           << "  WHERE EXISTS (SELECT 1 FROM " << quote_id (dtable) << endl
           << "    WHERE ";

        for (size_t i (0); i != rkey.contains_size (); ++i)
        {
          if (i != 0)
            os << endl
               << "      AND ";

          os << quote_id (rtable) << "." <<
            quote_id (rkey.contains_at (i).column ().name ()) << " = " <<
            quote_id (dtable) << "." <<
            quote_id (dkey.contains_at (i).column ().name ());
        }

        os << ")" << endl;

        post_statement ();
      }

      virtual void
      traverse (sema_rel::table& t, bool migration)
      {
        // By default drop foreign keys referencing tables that would
        // have already been dropped on the first pass.
        //
        if (pass_ == 1)
        {
          // Drop constraints. In migration this is always done on pass 1.
          //
          if (migration)
          {
            instance<drop_foreign_key> dfk (*this);
            trav_rel::unames n (*dfk);
            names (t, n);
          }
          else
          {
            dropped_.insert (t.name ()); // Add it before to cover self-refs.

            instance<drop_foreign_key> dfk (*this, dropped_);
            trav_rel::unames n (*dfk);
            names (t, n);
          }
        }
        else
        {
          if (migration && t.extra ()["kind"] == "polymorphic derived object")
          {
            // If we are dropping a polymorphic derived object, then we
            // also have to clean the base tables. Note that this won't
            // trigger cascade deletion since we have dropped all the
            // keys on pass 1. But we still need to do this in the base
            // to root order in order not to trigger other cascades.
            //
            using sema_rel::model;
            using sema_rel::table;
            using sema_rel::primary_key;
            using sema_rel::foreign_key;

            model& m (dynamic_cast<model&> (t.scope ()));

            table* p (&t);
            do
            {
              // The polymorphic link is the first primary key.
              //
              for (table::names_iterator i (p->names_begin ());
                   i != p->names_end (); ++i)
              {
                if (foreign_key* fk = dynamic_cast<foreign_key*> (
                      &i->nameable ()))
                {
                  p = m.find<table> (fk->referenced_table ());
                  assert (p != 0); // Base table should be there.
                  break;
                }
              }

              primary_key& rkey (*p->find<primary_key> (""));
              primary_key& dkey (*t.find<primary_key> (""));
              assert (rkey.contains_size () == dkey.contains_size ());
              delete_ (p->name (), t.name (), rkey, dkey);
            }
            while (p->extra ()["kind"] != "polymorphic root object");
          }

          drop (t, migration);
        }
      }

      virtual void
      traverse (sema_rel::table& t)
      {
        traverse (t, false);
      }

      virtual void
      traverse (sema_rel::drop_table& dt)
      {
        using sema_rel::model;
        using sema_rel::changeset;
        using sema_rel::table;

        // Find the table we are dropping in the base model.
        //
        changeset& cs (dynamic_cast<changeset&> (dt.scope ()));
        model& bm (cs.base_model ());
        table* t (bm.find<table> (dt.name ()));
        assert (t != 0);
        traverse (*t, true);
      }

      using add_table::traverse;   // Unhide.
      using alter_table::traverse; // Unhide.

      using table::names;

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
      table_set dropped_;
    };

    struct drop_model: trav_rel::model, common
    {
      typedef drop_model base;

      drop_model (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f)
      {
      }

      virtual void
      traverse (sema_rel::model& m)
      {
        // Traverse named entities in the reverse order. This way we
        // drop them in the order opposite to creating.
        //
        for (sema_rel::model::names_iterator begin (m.names_begin ()),
               end (m.names_end ()); begin != end;)
          dispatch (*--end);
      }

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
    };

    //
    // Create.
    //

    struct create_column: trav_rel::column,
                          trav_rel::add_column,
                          trav_rel::alter_column, // Override.
                          common
    {
      typedef create_column base;

      create_column (common const& c,
                     bool override_null = true,
                     bool* first = 0)
          : common (c),
            override_null_ (override_null),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true)
      {
      }

      create_column (create_column const& c)
          : root_context (), // @@ -Wextra
            context (),
            common (c),
            override_null_ (c.override_null_),
            first_ (&c.first_ != &c.first_data_ ? c.first_ : first_data_),
            first_data_ (c.first_data_)
      {
      }

      virtual void
      traverse (sema_rel::column& c)
      {
        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl
           << "  ";
        create (c);
      }

      virtual void
      add_header ()
      {
        // By default ADD COLUMN though some databases use just ADD.
        //
        os << "ADD COLUMN ";
      }

      virtual void
      traverse (sema_rel::add_column& ac)
      {
        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl
           << "  ";
        add_header ();
        create (ac);
      }

      virtual void
      create (sema_rel::column& c)
      {
        using sema_rel::column;

        // See if this column is (part of) a primary key.
        //
        sema_rel::primary_key* pk (0);

        for (column::contained_iterator i (c.contained_begin ());
             i != c.contained_end ();
             ++i)
        {
          if ((pk = dynamic_cast<sema_rel::primary_key*> (&i->key ())))
            break;
        }

        os << quote_id (c.name ()) << " ";

        type (c, pk != 0 && pk->auto_ ());
        constraints (c, pk);

        if (!c.options ().empty ())
          os << " " << c.options ();
      }

      virtual void
      constraints (sema_rel::column& c, sema_rel::primary_key* pk)
      {
        null (c);

        if (!c.default_ ().empty ())
          os << " DEFAULT " << c.default_ ();

        // If this is a single-column primary key, generate it inline.
        //
        if (pk != 0 && pk->contains_size () == 1)
          primary_key ();

        if (pk != 0 && pk->auto_ ())
          auto_ (*pk);
      }

      virtual void
      type (sema_rel::column& c, bool /*auto*/)
      {
        os << c.type ();
      }

      virtual void
      null (sema_rel::column& c)
      {
        bool n (c.null ());

        // If we are adding a new column that doesn't allow NULL nor has
        // a default value, add it as NULL. Later, after migration, we
        // will convert it to NOT NULL.
        //
        if (override_null_ && c.is_a<sema_rel::add_column> () &&
            !n && c.default_ ().empty ())
          n = true;

        // Specify both cases explicitly for better readability,
        // especially in ALTER COLUMN clauses.
        //
        os << (n ? " NULL" : " NOT NULL");
      }

      virtual void
      primary_key ()
      {
        os << " PRIMARY KEY";
      }

      virtual void
      auto_ (sema_rel::primary_key&)
      {
      }

    protected:
      bool override_null_; // Override NOT NULL in add_column.
      bool& first_;
      bool first_data_;
      bool add_;
    };

    struct create_primary_key: trav_rel::primary_key, common
    {
      typedef create_primary_key base;

      create_primary_key (common const& c): common (c) {}

      virtual void
      traverse (sema_rel::primary_key& pk)
      {
        // Single-column primary keys are generated inline in the
        // column declaration.
        //
        if (pk.contains_size () == 1)
          return;

        // We will always follow a column.
        //
        os << "," << endl;

        create (pk);
      }

      virtual void
      create (sema_rel::primary_key& pk)
      {
        using sema_rel::primary_key;

        // By default we create unnamed primary key constraint.
        //

        os << "  PRIMARY KEY (";

        for (primary_key::contains_iterator i (pk.contains_begin ());
             i != pk.contains_end ();
             ++i)
        {
          if (i != pk.contains_begin ())
            os << "," << endl
               << "               ";

          os << quote_id (i->column ().name ());
        }

        os << ")";
      }
    };

    struct create_foreign_key: trav_rel::foreign_key,
                               trav_rel::add_foreign_key,
                               common
    {
      typedef create_foreign_key base;

      // Schema constructor, pass 1.
      //
      create_foreign_key (common const& c, table_set& created, bool* first = 0)
          : common (c),
            created_ (&created),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true)
      {
      }

      // Schema constructor, pass 2 and migration constructor.
      //
      create_foreign_key (common const& c, bool* first = 0)
          : common (c),
            created_ (0),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true)
      {
      }

      create_foreign_key (create_foreign_key const& c)
          : root_context (), // @@ -Wextra
            context (),
            common (c),
            created_ (c.created_),
            first_ (&c.first_ != &c.first_data_ ? c.first_ : first_data_),
            first_data_ (c.first_data_)
      {
      }

      virtual void
      traverse (sema_rel::foreign_key& fk)
      {
        if (created_ != 0)
        {
          // Pass 1.
          //
          // If the referenced table has already been defined, do the
          // foreign key definition in the table definition. Otherwise
          // postpone it until pass 2 where we do it via ALTER TABLE.
          //
          if (created_->find (fk.referenced_table ()) != created_->end ())
          {
            traverse_create (fk);
            fk.set (db.string () + "-fk-defined", true); // Mark it as defined.
          }
        }
        else
        {
          // Pass 2.
          //
          if (!fk.count (db.string () + "-fk-defined"))
            traverse_add (fk);
        }
      }

      virtual void
      traverse_create (sema_rel::foreign_key& fk)
      {
        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl
           << "  CONSTRAINT ";
        create (fk);
      }

      virtual void
      traverse_add (sema_rel::foreign_key& fk)
      {
        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl;
        add (fk);
      }

      virtual void
      traverse (sema_rel::add_foreign_key& afk)
      {
        traverse_add (afk);
      }

      virtual void
      add_header ()
      {
        os << "ADD CONSTRAINT ";
      }

      virtual void
      add (sema_rel::foreign_key& fk)
      {
        os << "  ";
        add_header ();
        create (fk);
      }

      virtual void
      create (sema_rel::foreign_key& fk)
      {
        using sema_rel::foreign_key;

        os << name (fk) << endl
           << "    FOREIGN KEY (";

        for (foreign_key::contains_iterator i (fk.contains_begin ());
             i != fk.contains_end ();
             ++i)
        {
          if (i != fk.contains_begin ())
            os << "," << endl
               << "                 ";

          os << quote_id (i->column ().name ());
        }

        string tn (table_name (fk));
        string tn_pad (tn.size (), ' ');

        os << ")" << endl
           << "    REFERENCES " << tn << " (";

        foreign_key::columns const& refs (fk.referenced_columns ());
        for (foreign_key::columns::const_iterator i (refs.begin ());
             i != refs.end ();
             ++i)
        {
          if (i != refs.begin ())
            os << "," << endl
               << "                 " << tn_pad;

          os << quote_id (*i);
        }

        os << ")";

        if (fk.on_delete () != foreign_key::no_action)
          on_delete (fk.on_delete ());

        if (!fk.not_deferrable ())
          deferrable (fk.deferrable ());
      }

      virtual string
      name (sema_rel::foreign_key& fk)
      {
        return quote_id (fk.name ());
      }

      virtual string
      table_name (sema_rel::foreign_key& fk)
      {
        return quote_id (fk.referenced_table ());
      }

      virtual void
      on_delete (sema_rel::foreign_key::action_type a)
      {
        using sema_rel::foreign_key;

        switch (a)
        {
        case foreign_key::no_action:
          break;
        case foreign_key::cascade:
          {
            os << endl
               << "    ON DELETE CASCADE";
            break;
          }
        case foreign_key::set_null:
          {
            os << endl
               << "    ON DELETE SET NULL";
            break;
          }
        }
      }

      virtual void
      deferrable (sema_rel::deferrable d)
      {
        os << endl
           << "    DEFERRABLE INITIALLY " << d;
      }

    protected:
      table_set* created_;
      bool& first_;
      bool first_data_;
    };

    struct create_index: trav_rel::index, common
    {
      typedef create_index base;

      enum index_type {unique, non_unique, all};

      create_index (common const& c, index_type t = all)
          : common (c), type_ (t) {}

      virtual void
      traverse (sema_rel::index& in)
      {
        if (type_ == unique &&
            in.type ().find ("UNIQUE") == string::npos &&
            in.type ().find ("unique") == string::npos)
          return;

        if (type_ == non_unique && (
              in.type ().find ("UNIQUE") != string::npos ||
              in.type ().find ("unique") != string::npos))
          return;

        pre_statement ();
        create (in);
        post_statement ();
      }

      virtual string
      name (sema_rel::index& in)
      {
        return quote_id (in.name ());
      }

      virtual string
      table_name (sema_rel::index& in)
      {
        return quote_id (static_cast<sema_rel::table&> (in.scope ()).name ());
      }

      virtual void
      columns (sema_rel::index& in)
      {
        using sema_rel::index;

        for (index::contains_iterator i (in.contains_begin ());
             i != in.contains_end ();
             ++i)
        {
          if (in.contains_size () > 1)
          {
            if (i != in.contains_begin ())
              os << ",";

            os << endl
               << "    ";
          }

          os << quote_id (i->column ().name ());

          if (!i->options ().empty ())
            os << ' ' << i->options ();
        }
      }

      virtual void
      create (sema_rel::index& in)
      {
        // Default implementation that ignores the method.
        //
        os << "CREATE ";

        if (!in.type ().empty ())
          os << in.type () << ' ';

        os << "INDEX " << name (in) << endl
           << "  ON " << table_name (in) << " (";

        columns (in);

        os << ")" << endl;

        if (!in.options ().empty ())
          os << ' ' << in.options () << endl;
      }

    protected:
      index_type type_;
    };

    struct create_table: trav_rel::table,
                         trav_rel::alter_table, // Override.
                         common
    {
      typedef create_table base;

      using trav_rel::table::names;

      create_table (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f) {}

      virtual void
      create_pre (sema_rel::qname const& table)
      {
        os << "CREATE TABLE " << quote_id (table) << " (";
      }

      virtual void
      create_post (sema_rel::table& t)
      {
        os << ")" << endl;

        if (!t.options ().empty ())
          os << " " << t.options () << endl;
      }

      virtual void
      create (sema_rel::table& t)
      {
        pre_statement ();
        create_pre (t.name ());

        instance<create_column> c (*this);
        instance<create_primary_key> pk (*this);

        // We will always follow a column, so set first to false.
        //
        bool f (false); // (Im)perfect forwarding.
        bool* pf (&f);  // (Im)perfect forwarding.
        instance<create_foreign_key> fk (*this, created_, pf);

        trav_rel::unames n;
        n >> c;
        n >> pk;
        n >> fk;

        names (t, n);

        create_post (t);
        post_statement ();

        // Create indexes.
        //
        {
          instance<create_index> in (*this);
          trav_rel::unames n (*in);
          names (t, n);
        }
      }

      // See if there are any undefined foreign keys that we need to
      // add with ALTER TABLE.
      //
      bool
      check_undefined_fk (sema_rel::table& t)
      {
        for (sema_rel::table::names_iterator i (t.names_begin ());
             i != t.names_end (); ++i)
        {
          if (i->nameable ().is_a<sema_rel::foreign_key> () &&
              !i->nameable ().count (db.string () + "-fk-defined"))
            return true;
        }
        return false;
      }

      virtual void
      traverse (sema_rel::table& t)
      {
        // By default add foreign keys referencing tables that haven't
        // yet been defined on the second pass.
        //
        if (pass_ == 1)
        {
          // In migration we always add foreign keys on pass 2.
          //
          if (!t.is_a<sema_rel::add_table> ())
            created_.insert (t.name ()); // Add it before to cover self-refs.

          create (t);
        }
        else
        {
          // Add undefined foreign keys.
          //
          if (check_undefined_fk (t))
          {
            pre_statement ();
            os << "ALTER TABLE " << quote_id (t.name ());

            instance<create_foreign_key> cfk (*this);
            trav_rel::unames n (*cfk);
            names (t, n);
            os << endl;

            post_statement ();
          }
        }
      }

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
      table_set created_;
    };

    struct create_model: trav_rel::model, common
    {
      typedef create_model base;

      create_model (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f) {}

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
    };

    //
    // Alter.
    //

    struct alter_column: trav_rel::alter_column,
                         trav_rel::add_column,
                         common
    {
      typedef alter_column base;

      alter_column (common const& c, bool pre, bool* first = 0)
          : common (c),
            pre_ (pre),
            first_ (first != 0 ? *first : first_data_),
            first_data_ (true),
            fl_ (false),
            def_ (c, fl_)
      {
      }

      alter_column (alter_column const& c)
          : root_context (), // @@ -Wextra
            context (),
            common (c),
            pre_ (c.pre_),
            first_ (&c.first_ != &c.first_data_ ? c.first_ : first_data_),
            first_data_ (c.first_data_),
            fl_ (false),
            def_ (c, fl_)
      {
      }

      virtual void
      alter_header ()
      {
        os << "ALTER COLUMN ";
      }

      virtual void
      alter (sema_rel::column& c)
      {
        // By default use the whole definition.
        //
        def_->create (c);
      }

      virtual void
      traverse (sema_rel::column& c)
      {
        // Relax (NULL) in pre and tighten (NOT NULL) in post.
        //
        if (pre_ != c.null ())
          return;

        if (first_)
          first_ = false;
        else
          os << ",";

        os << endl
           << "  ";
        alter_header ();
        alter (c);
      }

      virtual void
      traverse (sema_rel::alter_column& ac)
      {
        assert (ac.null_altered ());
        traverse (static_cast<sema_rel::column&> (ac));
      }

      virtual void
      traverse (sema_rel::add_column& ac)
      {
        // We initially add NOT NULL columns without default values as
        // NULL. Now, after the migration, we convert them to NOT NULL.
        //
        if (!ac.null () && ac.default_ ().empty ())
          traverse (static_cast<sema_rel::column&> (ac));
      }

    protected:
      bool pre_;
      bool& first_;
      bool first_data_;
      bool fl_; // (Im)perfect forwarding.
      instance<create_column> def_;
    };

    struct alter_table_common: trav_rel::alter_table, common
    {
      alter_table_common (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f) {}

      template <typename T>
      T*
      check (sema_rel::alter_table& at)
      {
        for (sema_rel::alter_table::names_iterator i (at.names_begin ());
             i != at.names_end (); ++i)
        {
          if (T* x = dynamic_cast<T*> (&i->nameable ()))
            return x;
        }
        return 0;
      }

      sema_rel::column*
      check_alter_column_null (sema_rel::alter_table& at, bool v)
      {
        for (sema_rel::alter_table::names_iterator i (at.names_begin ());
             i != at.names_end (); ++i)
        {
          using sema_rel::add_column;
          using sema_rel::alter_column;

          if (alter_column* ac = dynamic_cast<alter_column*> (&i->nameable ()))
          {
            if (ac->null_altered () && ac->null () == v)
              return ac;
          }

          // If we are testing for NOT NULL, also look for new columns that
          // we initially add as NULL and later convert to NOT NULL.
          //
          if (!v)
          {
            if (add_column* ac = dynamic_cast<add_column*> (&i->nameable ()))
            {
              if (!ac->null () && ac->default_ ().empty ())
                return ac;
            }
          }
        }
        return 0;
      }

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
    };

    struct alter_table_pre: alter_table_common
    {
      typedef alter_table_pre base;

      alter_table_pre (emitter_type& e, ostream& os, schema_format f)
          : alter_table_common (e, os, f) {}

      // Check if there will be any clauses in ALTER TABLE.
      //
      using alter_table_common::check;

      virtual bool
      check (sema_rel::alter_table& at)
      {
        // If changing the below test, make sure to also update tests
        // in database-specific code.
        //
        return
          check<sema_rel::drop_foreign_key> (at) ||
          check<sema_rel::add_column> (at) ||
          check_alter_column_null (at, true);
      }

      virtual void
      alter (sema_rel::alter_table& at)
      {
        // By default we generate all the alterations in a single ALTER TABLE
        // statement. Quite a few databases don't support this.
        //
        pre_statement ();
        os << "ALTER TABLE " << quote_id (at.name ());

        bool f (true);  // Shared first flag.
        bool* pf (&f);  // (Im)perfect forwarding.
        bool tl (true); // (Im)perfect forwarding.
        instance<create_column> cc (*this, tl, pf);
        instance<alter_column> ac (*this, tl, pf);
        instance<drop_foreign_key> dfk (*this, pf);
        trav_rel::unames n;
        n >> cc;
        n >> ac;
        n >> dfk;
        names (at, n);
        os << endl;

        post_statement ();
      }

      virtual void
      traverse (sema_rel::alter_table& at)
      {
        if (pass_ == 1)
        {
          // Drop unique indexes.
          //
          {
            drop_index::index_type it (drop_index::unique);
            instance<drop_index> in (*this, it);
            trav_rel::unames n (*in);
            names (at, n);
          }

          if (check (at))
            alter (at);
        }
        else
        {
          // Add non-unique indexes.
          //
          {
            create_index::index_type it (create_index::non_unique);
            instance<create_index> in (*this, it);
            trav_rel::unames n (*in);
            names (at, n);
          }
        }
      }
    };

    struct changeset_pre: trav_rel::changeset, common
    {
      typedef changeset_pre base;

      changeset_pre (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f) {}

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
    };

    struct alter_table_post: alter_table_common
    {
      typedef alter_table_post base;

      alter_table_post (emitter_type& e, ostream& os, schema_format f)
          : alter_table_common (e, os, f) {}

      // Check if there will be any clauses in ALTER TABLE.
      //
      using alter_table_common::check;

      virtual bool
      check (sema_rel::alter_table& at)
      {
        // If changing the below test, make sure to also update tests
        // in database-specific code.
        //
        return
          check<sema_rel::add_foreign_key> (at) ||
          check<sema_rel::drop_column> (at) ||
          check_alter_column_null (at, false);
      }

      virtual void
      alter (sema_rel::alter_table& at)
      {
        // By default we generate all the alterations in a single ALTER TABLE
        // statement. Quite a few databases don't support this.
        //
        pre_statement ();
        os << "ALTER TABLE " << quote_id (at.name ());

        bool f (true); // Shared first flag.
        bool* pf (&f); // (Im)perfect forwarding.
        bool fl (false); // (Im)perfect forwarding.
        instance<drop_column> dc (*this, pf);
        instance<alter_column> ac (*this, fl, pf);
        instance<create_foreign_key> fk (*this, pf);

        trav_rel::unames n;
        n >> dc;
        n >> ac;
        n >> fk;
        names (at, n);
        os << endl;

        post_statement ();
      }

      virtual void
      traverse (sema_rel::alter_table& at)
      {
        if (pass_ == 1)
        {
          // Drop non-unique indexes.
          //
          {
            drop_index::index_type it (drop_index::non_unique);
            instance<drop_index> in (*this, it);
            trav_rel::unames n (*in);
            names (at, n);
          }
        }
        else
        {
          if (check (at))
            alter (at);

          // Add unique indexes.
          //
          {
            create_index::index_type it (create_index::unique);
            instance<create_index> in (*this, it);
            trav_rel::unames n (*in);
            names (at, n);
          }
        }
      }
    };

    struct changeset_post: trav_rel::changeset, common
    {
      typedef changeset_post base;

      changeset_post (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f) {}

      virtual void
      traverse (sema_rel::changeset& m)
      {
        // Traverse named entities in the reverse order. This way we
        // drop them in the order opposite to creating.
        //
        for (sema_rel::changeset::names_iterator begin (m.names_begin ()),
               end (m.names_end ()); begin != end;)
          dispatch (*--end);
      }

      void
      pass (unsigned short p)
      {
        pass_ = p;
      }

    protected:
      unsigned short pass_;
    };

    //
    // Schema version table.
    //

    struct version_table: common
    {
      typedef version_table base;

      version_table (emitter_type& e, ostream& os, schema_format f)
          : common (e, os, f),
            table_ (options.schema_version_table ()[db]),
            qt_ (quote_id (table_)),
            qs_ (quote_string (options.schema_name ()[db])),
            qn_ (quote_id ("name")),
            qv_ (quote_id ("version")),
            qm_ (quote_id ("migration"))
      {
      }

      // Create the version table if it doesn't exist.
      //
      virtual void
      create_table () {}

      // Remove the version entry. Called after the DROP statements.
      //
      virtual void
      drop ()
      {
        pre_statement ();

        os << "DELETE FROM " << qt_ << endl
           << "  WHERE " << qn_ << " = " << qs_ << endl;

        post_statement ();
      }

      // Set the version. Called after the CREATE statements.
      //
      virtual void
      create (sema_rel::version) {}

      // Set the version and migration state to true. Called after
      // the pre migration statements.
      //
      virtual void
      migrate_pre (sema_rel::version v)
      {
        pre_statement ();

        os << "UPDATE " << qt_ << endl
           << "  SET " << qv_ << " = " << v << ", " << qm_ << " = 1" << endl
           << "  WHERE " << qn_ << " = " << qs_ << endl;

        post_statement ();
      }

      // Set migration state to false. Called after the post migration
      // statements.
      //
      virtual void
      migrate_post ()
      {
        pre_statement ();

        os << "UPDATE " << qt_ << endl
           << "  SET " << qm_ << " = 0" << endl
           << "  WHERE " << qn_ << " = " << qs_ << endl;

        post_statement ();
      }

    protected:
      sema_rel::qname table_;
      string qt_; // Quoted table.
      string qs_; // Quoted schema name string.
      string qn_; // Quoted name column.
      string qv_; // Quoted version column.
      string qm_; // Quoted migration column.
    };

    //
    // SQL output.
    //

    struct sql_emitter: emitter, virtual context
    {
      typedef sql_emitter base;

      virtual void
      pre ()
      {
        first_ = true;
      }

      virtual void
      line (const std::string& l)
      {
        if (first_ && !l.empty ())
          first_ = false;
        else
          os << endl;

        os << l;
      }

      virtual void
      post ()
      {
        if (!first_) // Ignore empty statements.
          os << ';' << endl
             << endl;
      }

    protected:
      bool first_;
    };

    struct sql_file: virtual context
    {
      typedef sql_file base;

      virtual void
      prologue ()
      {
      }

      virtual void
      epilogue ()
      {
      }
    };
  }
}

#endif // ODB_RELATIONAL_SCHEMA_HXX
