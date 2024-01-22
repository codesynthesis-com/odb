// file      : odb/relational/mssql/schema.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/schema.hxx>

#include <odb/relational/mssql/common.hxx>
#include <odb/relational/mssql/context.hxx>

using namespace std;

namespace relational
{
  namespace mssql
  {
    namespace schema
    {
      namespace relational = relational::schema;
      using relational::table_set;

      struct sql_emitter: relational::sql_emitter
      {
        sql_emitter (const base& x): base (x) {}

        virtual void
        post ()
        {
          if (!first_) // Ignore empty statements.
          {
            os << ';' << endl
               << "GO" << endl
               << endl;
          }
        }
      };
      entry<sql_emitter> sql_emitter_;

      //
      // File.
      //

      struct sql_file: relational::sql_file, context
      {
        sql_file (const base& x): base (x) {}

        virtual void
        prologue ()
        {
          // Suppress the (x rows affected) messages from sqlcmd for DML
          // statements. We only use DML for schema version management.
          //
          if ((model == 0 || model->version () != 0) &&
              !options.suppress_schema_version ())
            os << "SET NOCOUNT ON;" << endl
               << endl;
        }
      };
      entry<sql_file> sql_file_;

      //
      // Drop.
      //

      struct drop_column: relational::drop_column, context
      {
        drop_column (base const& x): base (x) {}

        virtual void
        traverse (sema_rel::drop_column& dc)
        {
          if (first_)
            first_ = false;
          else
            os << "," << endl
               << "              ";

          os << quote_id (dc.name ());
        }
      };
      entry<drop_column> drop_column_;

      struct drop_foreign_key: relational::drop_foreign_key, context
      {
        drop_foreign_key (base const& x): base (x) {}

        virtual void
        drop (sema_rel::table& t, sema_rel::foreign_key& fk)
        {
          bool migration (dropped_ == 0);

          if (migration)
          {
            if (fk.not_deferrable ())
              pre_statement ();
            else
            {
              if (format_ != schema_format::sql)
                return;

              os << "/*" << endl;
            }
          }
          else
          {
            // Here we drop potentially deferrable keys and also need to
            // test if the key exists.
            //
            pre_statement ();

            os << "IF OBJECT_ID(" << quote_string (fk.name ()) << ", " <<
              quote_string ("F") << ") IS NOT NULL" << endl
               << "  ";
          }

          os << "ALTER TABLE " << quote_id (t.name ()) << endl
             << (migration ? "  " : "    ") << "DROP CONSTRAINT " <<
            quote_id (fk.name ()) << endl;


          if (!migration || fk.not_deferrable ())
            post_statement ();
          else
            os << "*/" << endl
               << endl;
        }

        virtual void
        traverse (sema_rel::drop_foreign_key& dfk)
        {
          // Find the foreign key we are dropping in the base model.
          //
          sema_rel::foreign_key& fk (find<sema_rel::foreign_key> (dfk));

          bool c (!fk.not_deferrable () && !in_comment);

          if (c && format_ != schema_format::sql)
            return;

          if (!first_)
            os << (c ? "" : ",") << endl
               << "                  ";

          if (c)
            os << "/* ";

          os << quote_id (fk.name ());

          if (c)
            os << " */";

          if (first_)
          {
            if (c)
              // There has to be a real name otherwise the whole statement
              // would have been commented out.
              //
              os << endl
                 << "                  ";
            else
              first_ = false;
          }
        }
      };
      entry<drop_foreign_key> drop_foreign_key_;

      struct drop_index: relational::drop_index, context
      {
        drop_index (base const& x): base (x) {}

        virtual void
        drop (sema_rel::index& in)
        {
          sema_rel::table& t (static_cast<sema_rel::table&> (in.scope ()));

          os << "DROP INDEX " << name (in) << " ON " <<
            quote_id (t.name ()) << endl;
        }
      };
      entry<drop_index> drop_index_;

      struct drop_table: relational::drop_table, context
      {
        drop_table (base const& x): base (x) {}

        virtual void
        drop (sema_rel::table& t, bool migration)
        {
          // SQL Server has no IF EXISTS conditional for dropping tables.
          // The following approach appears to be the recommended way to
          // drop a table if it exists.
          //
          sema_rel::qname const& name (t.name ());

          pre_statement ();

          if (!migration)
            os << "IF OBJECT_ID(" << quote_string (name.string ()) <<
              ", " << quote_string ("U") << ") IS NOT NULL" << endl
               << "  ";

          os << "DROP TABLE " << quote_id (name) << endl;

          post_statement ();
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
          if (first_)
            first_ = false;
          else
            os << "," << endl
               << "      ";

          create (ac);
        }

        virtual void
        auto_ (sema_rel::primary_key&)
        {
          os << " IDENTITY";
        }
      };
      entry<create_column> create_column_;

      struct create_foreign_key: relational::create_foreign_key, context
      {
        create_foreign_key (base const& x): base (x) {}

        void
        diagnose (sema_rel::foreign_key& fk)
        {
          if (fk.on_delete () != sema_rel::foreign_key::no_action)
          {
            cerr << "warning: foreign key '" << fk.name () << "' has " <<
              "ON DELETE clause but is disabled in SQL Server due to lack "
              "of deferrable constraint support" << endl;

            cerr << "info: consider using non-deferrable foreign keys (" <<
              "--fkeys-deferrable-mode)" << endl;
          }
        }

        virtual void
        traverse_create (sema_rel::foreign_key& fk)
        {
          // SQL Server does not support deferrable constraint checking.
          // Output such foreign keys as comments, for documentation,
          // unless we are generating embedded schema.
          //
          if (fk.not_deferrable ())
            base::traverse_create (fk);
          else
          {
            diagnose (fk);

            // Don't bloat C++ code with comment strings if we are
            // generating embedded schema.
            //
            if (format_ != schema_format::sql)
              return;

            os << endl
               << "  /*" << endl
               << "  CONSTRAINT ";
            create (fk);
            os << endl
               << "  */";
          }
        }

        virtual void
        traverse_add (sema_rel::foreign_key& fk)
        {
          bool c (!fk.not_deferrable () && !in_comment);

          if (c)
            diagnose (fk);

          if (c && format_ != schema_format::sql)
            return;

          if (!first_)
            os << (c ? "" : ",") << endl
               << "      ";

          if (c)
            os << "/*" << endl
               << "      ";

          os << "CONSTRAINT ";
          create (fk);

          if (c)
            os << endl
               << "      */";

          if (first_)
          {
            if (c)
              // There has to be a real key otherwise the whole statement
              // would have been commented out.
              //
              os << endl
                 << "      ";
            else
              first_ = false;
          }
        }

        virtual void
        deferrable (sema_rel::deferrable)
        {
          // This will still be called to output the comment.
        }
      };
      entry<create_foreign_key> create_foreign_key_;

      struct create_table: relational::create_table, context
      {
        create_table (base const& x): base (x) {}

        // See if there are any undefined foreign keys that are not
        // deferrable.
        //
        bool
        check_undefined_fk_deferrable_only (sema_rel::table& t)
        {
          for (sema_rel::table::names_iterator i (t.names_begin ());
               i != t.names_end (); ++i)
          {
            using sema_rel::foreign_key;

            if (foreign_key* fk = dynamic_cast<foreign_key*> (&i->nameable ()))
            {
              if (!fk->count ("mssql-fk-defined") &&
                  fk->not_deferrable ())
                return false;
            }
          }
          return true;
        }

        virtual void
        traverse (sema_rel::table& t)
        {
          if (pass_ == 1)
            base::traverse (t);
          else
          {
            // Add undefined foreign keys.
            //
            if (check_undefined_fk (t))
            {
              bool deferrable (check_undefined_fk_deferrable_only (t));

              if (!deferrable || format_ == schema_format::sql)
              {
                if (deferrable)
                {
                  os << "/*" << endl;
                  in_comment = true;
                }
                else
                  pre_statement ();

                os << "ALTER TABLE " << quote_id (t.name ()) << endl
                   << "  ADD ";

                instance<create_foreign_key> cfk (*this);
                trav_rel::unames n (*cfk);
                names (t, n);
                os << endl;

                if (deferrable)
                {
                  in_comment = false;
                  os << "*/" << endl
                     << endl;
                }
                else
                  post_statement ();
              }
            }
          }
        }
      };
      entry<create_table> create_table_;

      //
      // Alter.
      //

      struct alter_column: relational::alter_column, context
      {
        alter_column (base const& x): base (x) {}

        virtual void
        traverse (sema_rel::column& c)
        {
          // Relax (NULL) in pre and tighten (NOT NULL) in post.
          //
          if (pre_ != c.null ())
            return;

          using sema_rel::table;
          table& at (static_cast<table&> (c.scope ()));

          pre_statement ();

          os << "ALTER TABLE " << quote_id (at.name ()) << endl
             << "  ALTER COLUMN ";
          alter (c);
          os << endl;

          post_statement ();
        }
      };
      entry<alter_column> alter_column_;

      struct alter_table_pre: relational::alter_table_pre, context
      {
        alter_table_pre (base const& x): base (x) {}

        // Check if we are only dropping deferrable foreign keys.
        //
        bool
        check_drop_deferrable_only (sema_rel::alter_table& at)
        {
          for (sema_rel::alter_table::names_iterator i (at.names_begin ());
               i != at.names_end (); ++i)
          {
            using sema_rel::foreign_key;
            using sema_rel::drop_foreign_key;

            if (drop_foreign_key* dfk =
                dynamic_cast<drop_foreign_key*> (&i->nameable ()))
            {
              foreign_key& fk (find<foreign_key> (*dfk));

              if (fk.not_deferrable ())
                return false;
            }
          }
          return true;
        }

        virtual void
        alter (sema_rel::alter_table& at)
        {
          // SQL Server can only alter one kind of thing at a time.
          //
          if (check<sema_rel::drop_foreign_key> (at))
          {
            bool deferrable (check_drop_deferrable_only (at));

            if (!deferrable || format_ == schema_format::sql)
            {
              if (deferrable)
              {
                os << "/*" << endl;
                in_comment = true;
              }
              else
                pre_statement ();

              os << "ALTER TABLE " << quote_id (at.name ()) << endl
                 << "  DROP CONSTRAINT ";

              instance<drop_foreign_key> dfc (*this);
              trav_rel::unames n (*dfc);
              names (at, n);
              os << endl;

              if (deferrable)
              {
                in_comment = false;
                os << "*/" << endl
                   << endl;
              }
              else
                post_statement ();
            }
          }

          if (check<sema_rel::add_column> (at))
          {
            pre_statement ();

            os << "ALTER TABLE " << quote_id (at.name ()) << endl
               << "  ADD ";

            instance<create_column> cc (*this);
            trav_rel::unames n (*cc);
            names (at, n);
            os << endl;

            post_statement ();
          }

          // For ALTER COLUMN, SQL Server can only have one per ALTER TABLE.
          //
          {
            bool tl (true); // (Im)perfect forwarding.
            instance<alter_column> ac (*this, tl);
            trav_rel::unames n (*ac);
            names (at, n);
          }
        }
      };
      entry<alter_table_pre> alter_table_pre_;

      struct alter_table_post: relational::alter_table_post, context
      {
        alter_table_post (base const& x): base (x) {}

        // Check if we are only adding deferrable foreign keys.
        //
        bool
        check_add_deferrable_only (sema_rel::alter_table& at)
        {
          for (sema_rel::alter_table::names_iterator i (at.names_begin ());
               i != at.names_end (); ++i)
          {
            using sema_rel::add_foreign_key;

            if (add_foreign_key* afk =
                dynamic_cast<add_foreign_key*> (&i->nameable ()))
            {
              if (afk->not_deferrable ())
                return false;
            }
          }
          return true;
        }

        virtual void
        alter (sema_rel::alter_table& at)
        {
          // SQL Server can only alter one kind of thing at a time.
          //
          if (check<sema_rel::drop_column> (at))
          {
            pre_statement ();

            os << "ALTER TABLE " << quote_id (at.name ()) << endl
               << "  DROP COLUMN ";

            instance<drop_column> dc (*this);
            trav_rel::unames n (*dc);
            names (at, n);
            os << endl;

            post_statement ();
          }

          // For ALTER COLUMN, SQL Server can only have one per ALTER TABLE.
          //
          {
            bool fl (false); // (Im)perfect forwarding.
            instance<alter_column> ac (*this, fl);
            trav_rel::unames n (*ac);
            names (at, n);
          }

          if (check<sema_rel::add_foreign_key> (at))
          {
            bool deferrable (check_add_deferrable_only (at));

            if (!deferrable || format_ == schema_format::sql)
            {
              if (deferrable)
              {
                os << "/*" << endl;
                in_comment = true;
              }
              else
                pre_statement ();

              os << "ALTER TABLE " << quote_id (at.name ()) << endl
                 << "  ADD ";

              instance<create_foreign_key> cfc (*this);
              trav_rel::unames n (*cfc);
              names (at, n);
              os << endl;

              if (deferrable)
              {
                in_comment = false;
                os << "*/" << endl
                   << endl;
              }
              else
                post_statement ();
            }
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

          os << "IF OBJECT_ID(" << quote_string (table_.string ()) <<
            ", " << quote_string ("U") << ") IS NULL" << endl
             << "  CREATE TABLE " << qt_ << " (" << endl
             << "    " << qn_ << " VARCHAR(256) NOT NULL PRIMARY KEY," << endl
             << "    " << qv_ << " BIGINT NOT NULL," << endl
             << "    " << qm_ << " BIT NOT NULL)" << endl;

          post_statement ();
        }

        virtual void
        create (sema_rel::version v)
        {
          pre_statement ();

          os << "IF NOT EXISTS (SELECT 1 FROM " << qt_ << " WHERE " << qn_ <<
            " = " << qs_ << ")" << endl
             << "  INSERT INTO " << qt_ << " (" << endl
             << "    " << qn_ << ", " << qv_ << ", " << qm_ << ")" << endl
             << "    VALUES (" << qs_ << ", " << v << ", 0)" << endl;

          post_statement ();
        }
      };
      entry<version_table> version_table_;
    }
  }
}
