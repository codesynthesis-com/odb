// file      : odb/relational/sqlite/schema.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/schema.hxx>

#include <odb/relational/sqlite/common.hxx>
#include <odb/relational/sqlite/context.hxx>

namespace relational
{
  namespace sqlite
  {
    namespace schema
    {
      namespace relational = relational::schema;

      //
      // Drop.
      //

      struct drop_column: trav_rel::drop_column, relational::common
      {
        drop_column (relational::common const& c)
            : relational::common (c), first_ (true) {}

        virtual void
        traverse (sema_rel::drop_column& dc)
        {
          // SQLite does not support dropping columns. If this column is
          // not NULLable, then there is nothing we can do. Otherwise, do
          // a logical DROP by setting all the values to NULL.
          //
          sema_rel::column& c (find<sema_rel::column> (dc));

          if (!c.null ())
          {
            cerr << "error: SQLite does not support dropping of columns" <<
              endl;
            cerr << "info: first dropped column is '" << dc.name () <<
              "' in table '" << dc.table ().name () << "'" << endl;
            cerr << "info: could have performed logical drop if the column " <<
              "allowed NULL values" << endl;
            throw operation_failed ();
          }

          if (first_)
            first_ = false;
          else
            os << "," << endl
               << "    ";

          os << quote_id (dc.name ()) << " = NULL";
        }

      private:
        bool first_;
      };
      // Not registered as an override.

      struct drop_index: relational::drop_index, context
      {
        drop_index (base const& x): base (x) {}

        virtual string
        name (sema_rel::index& in)
        {
          // In SQLite, index names can be qualified with the database.
          //
          sema_rel::table& t (static_cast<sema_rel::table&> (in.scope ()));
          sema_rel::qname n (t.name ().qualifier ());
          n.append (in.name ());
          return quote_id (n);
        }
      };
      entry<drop_index> drop_index_;

      struct drop_table: relational::drop_table, context
      {
        drop_table (base const& x): base (x) {}

        virtual void
        traverse (sema_rel::table& t, bool migration)
        {
          // In SQLite there is no way to drop foreign keys except as part
          // of the table.
          //
          if (pass_ != 2)
            return;

          // Polymorphic base cleanup code. Because we cannot drop foreign
          // keys, we will trigger cascade deletion. The only way to work
          // around this problem is to delete from the root table and rely
          // on the cascade to clean up the rest.
          //
          if (migration && t.extra ()["kind"] == "polymorphic derived object")
          {
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
            }
            while (p->extra ()["kind"] != "polymorphic root object");

            primary_key& rkey (*p->find<primary_key> (""));
            primary_key& dkey (*t.find<primary_key> (""));
            assert (rkey.contains_size () == dkey.contains_size ());
            delete_ (p->name (), t.name (), rkey, dkey);
          }

          drop (t, migration);
        }
      };
      entry<drop_table> drop_table_;

      //
      // Create.
      //

      struct create_column: relational::create_column, context
      {
        create_column (base const& x): base (x) {}

        virtual void
        traverse (sema_rel::add_column& ac)
        {
          using sema_rel::alter_table;
          using sema_rel::add_column;
          using sema_rel::add_foreign_key;

          alter_table& at (static_cast<alter_table&> (ac.scope ()));

          pre_statement ();

          os << "ALTER TABLE " << quote_id (at.name ()) << endl
             << "  ADD COLUMN ";

          // In SQLite it is impossible to alter a column later, so unless
          // it has a default value, we add it as NULL. Without this, it
          // will be impossible to add a column to a table that contains
          // some rows.
          //
          create (ac);

          // SQLite doesn't support adding foreign keys other than inline
          // via a column definition. See if we can handle any.
          //
          add_foreign_key* afk (0);

          for (add_column::contained_iterator i (ac.contained_begin ());
               i != ac.contained_end ();
               ++i)
          {
            if ((afk = dynamic_cast<add_foreign_key*> (&i->key ())))
            {
              // Check that it is a single-column foreign key. Also make
              // sure the column and foreign key are from the same changeset.
              //
              if (afk->contains_size () != 1 || &ac.scope () != &afk->scope ())
                afk = 0;
              else
                break;
            }
          }

          if (afk != 0)
          {
            os << " CONSTRAINT " << quote_id (afk->name ()) << " REFERENCES " <<
              quote_id (afk->referenced_table ().uname ()) << " (" <<
              quote_id (afk->referenced_columns ()[0]) << ")";

            bool del (afk->on_delete () != sema_rel::foreign_key::no_action);
            bool def (!afk->not_deferrable ());

            if (del || def)
            {
              instance<relational::create_foreign_key> cfk (*this);

              if (del)
                cfk->on_delete (afk->on_delete ());

              if (def)
                cfk->deferrable (afk->deferrable ());
            }

            afk->set ("sqlite-fk-defined", true); // Mark it as defined.
          }

          os << endl;
          post_statement ();
        }

        virtual void
        auto_ (sema_rel::primary_key& pk)
        {
          if (pk.extra ().count ("lax"))
            os << " /*AUTOINCREMENT*/";
          else
            os << " AUTOINCREMENT";
        }
      };
      entry<create_column> create_column_;

      struct create_foreign_key: relational::create_foreign_key, context
      {
        create_foreign_key (base const& x): base (x) {}

        virtual void
        traverse (sema_rel::foreign_key& fk)
        {
          // In SQLite, all constraints are defined as part of a table.
          //
          os << "," << endl
             << "  CONSTRAINT ";

          create (fk);
        }

        virtual string
        table_name (sema_rel::foreign_key& fk)
        {
          // In SQLite, the referenced table cannot be qualified with the
          // database name (it has to be in the same database anyway).
          //
          return quote_id (fk.referenced_table ().uname ());
        }
      };
      entry<create_foreign_key> create_foreign_key_;

      struct create_index: relational::create_index, context
      {
        create_index (base const& x): base (x) {}

        virtual string
        name (sema_rel::index& in)
        {
          // In SQLite, index names can be qualified with the database.
          //
          sema_rel::table& t (static_cast<sema_rel::table&> (in.scope ()));
          sema_rel::qname n (t.name ().qualifier ());
          n.append (in.name ());
          return quote_id (n);
        }

        virtual string
        table_name (sema_rel::index& in)
        {
          // In SQLite, the index table cannot be qualified with the
          // database name (it has to be in the same database).
          //
          return quote_id (
            static_cast<sema_rel::table&> (in.scope ()).name ().uname ());
        }
      };
      entry<create_index> create_index_;

      struct create_table: relational::create_table, context
      {
        create_table (base const& x): base (x) {}

        void
        traverse (sema_rel::table& t)
        {
          // For SQLite we do everything in a single pass since there
          // is no way to add constraints later.
          //
          if (pass_ == 1)
            create (t);
        }
      };
      entry<create_table> create_table_;

      //
      // Alter.
      //

      struct alter_table_pre: relational::alter_table_pre, context
      {
        alter_table_pre (base const& x): base (x) {}

        virtual void
        alter (sema_rel::alter_table& at)
        {
          // SQLite can only add a single column per ALTER TABLE statement.
          //
          instance<create_column> cc (*this);
          trav_rel::unames n (*cc);
          names (at, n);

          // SQLite does not support altering columns.
          //
          if (sema_rel::alter_column* ac = check<sema_rel::alter_column> (at))
          {
            cerr << "error: SQLite does not support altering of columns"
                 << endl;
            cerr << "info: first altered column is '" << ac->name () <<
              "' in table '" << at.name () << "'" << endl;
            throw operation_failed ();
          }

          // SQLite does not support dropping constraints. We are going to
          // ignore this if the column is NULL'able since in most cases
          // the constraint is going to be dropped as a result of the
          // column drop (e.g., an object pointer member got deleted).
          // If we were not to allow this, then it would be impossible
          // to do logical drop for pointer columns.
          //
          for (sema_rel::alter_table::names_iterator i (at.names_begin ());
               i != at.names_end (); ++i)
          {
            using sema_rel::foreign_key;
            using sema_rel::drop_foreign_key;

            drop_foreign_key* dfk (
              dynamic_cast<drop_foreign_key*> (&i->nameable ()));

            if (dfk == 0)
              continue;

            foreign_key& fk (find<foreign_key> (*dfk));

            for (foreign_key::contains_iterator j (fk.contains_begin ());
                 j != fk.contains_end (); ++j)
            {
              if (j->column ().null ())
                continue;

              cerr << "error: SQLite does not support dropping of foreign " <<
                "keys" << endl;
              cerr << "info: first dropped foreign key is '" << dfk->name () <<
                "' in table '" << at.name () << "'" << endl;
              cerr << "info: could have ignored it if the contained " <<
                "column(s) allowed NULL values" << endl;
              throw operation_failed ();
            }
          }
        }
      };
      entry<alter_table_pre> alter_table_pre_;

      struct alter_table_post: relational::alter_table_post, context
      {
        alter_table_post (base const& x): base (x) {}

        virtual void
        alter (sema_rel::alter_table& at)
        {
          // SQLite does not support altering columns (we have to do this
          // in both alter_table_pre/post because of the
          // check_alter_column_null() test in the common code).
          //
          if (sema_rel::alter_column* ac = check<sema_rel::alter_column> (at))
          {
            cerr << "error: SQLite does not support altering of columns"
                 << endl;
            cerr << "info: first altered column is '" << ac->name () <<
              "' in table '" << at.name () << "'" << endl;
            throw operation_failed ();
          }

          // Try to do logical column drop.
          //
          if (check<sema_rel::drop_column> (at))
          {
            pre_statement ();

            os << "UPDATE " << quote_id (at.name ()) << endl
               << "  SET ";

            drop_column dc (*this);
            trav_rel::unames n (dc);
            names (at, n);
            os << endl;

            post_statement ();
          }

          // SQLite doesn't support adding foreign keys other than inline
          // via a column definition. See if there are any that we couldn't
          // handle that way.
          //
          for (sema_rel::alter_table::names_iterator i (at.names_begin ());
               i != at.names_end (); ++i)
          {
            sema_rel::add_foreign_key* afk (
              dynamic_cast<sema_rel::add_foreign_key*> (&i->nameable ()));

            if (afk == 0 || afk->count ("sqlite-fk-defined"))
              continue;

            cerr << "error: SQLite does not support adding foreign keys"
                 << endl;
            cerr << "info: first added foreign key is '" << afk->name () <<
              "' in table '" << at.name () << "'" << endl;
            throw operation_failed ();
          }
        }
      };
      entry<alter_table_post> alter_table_post_;

      //
      // Schema version table.
      //

      struct version_table: relational::version_table, context
      {
        version_table (base const& x): base (x) {}

        virtual void
        create_table ()
        {
          pre_statement ();

          os << "CREATE TABLE IF NOT EXISTS " << qt_ << " (" << endl
             << "  " << qn_ << " TEXT NOT NULL PRIMARY KEY," << endl
             << "  " << qv_ << " INTEGER NOT NULL," << endl
             << "  " << qm_ << " INTEGER NOT NULL)" << endl;

          post_statement ();
        }

        virtual void
        create (sema_rel::version v)
        {
          pre_statement ();

          os << "INSERT OR IGNORE INTO " << qt_ << " (" << endl
             << "  " << qn_ << ", " << qv_ << ", " << qm_ << ")" << endl
             << "  VALUES (" << qs_ << ", " << v << ", 0)" << endl;

          post_statement ();
        }
      };
      entry<version_table> version_table_;
    }
  }
}
