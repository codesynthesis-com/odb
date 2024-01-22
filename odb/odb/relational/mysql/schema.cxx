// file      : odb/relational/mysql/schema.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/schema.hxx>

#include <odb/relational/mysql/common.hxx>
#include <odb/relational/mysql/context.hxx>

using namespace std;

namespace relational
{
  namespace mysql
  {
    namespace schema
    {
      namespace relational = relational::schema;
      using relational::table_set;

      //
      // Drop.
      //

      struct drop_foreign_key: relational::drop_foreign_key, context
      {
        drop_foreign_key (base const& x): base (x) {}

        virtual void
        drop (sema_rel::table& t, sema_rel::foreign_key& fk)
        {
          /*
          // @@ This does not work: in MySQL control statements can only
          //    be used in stored procedures. It seems the only way to
          //    implement this is to define, execute, and drop a stored
          //    procedure, which is just too ugly.
          //
          //    Another option would be to use CREATE TABLE IF NOT EXISTS
          //    to create a dummy table with a dummy constraint that makes
          //    the following DROP succeed. Note, however, that MySQL issues
          //    a notice if the table already exist so would need to suppress
          //    that as well. Still not sure that the utility of this support
          //    justifies this kind of a hack.
          //
          os << "IF EXISTS (SELECT NULL FROM information_schema.TABLE_CONSTRAINTS" << endl
             << "    WHERE CONSTRAINT_TYPE = " << quote_string ("FOREIGN KEY") << "AND" << endl
             << "          CONSTRAINT_SCHEMA = DATABASE() AND" << endl
             << "          CONSTRAINT_NAME = " << quote_string (fk.name ()) << ") THEN" << endl
             << "  ALTER TABLE " << quote_id (t.name ()) << " DROP FOREIGN KEY " << quote_id (fk.name ()) << ";" << endl
             << "END IF;" << endl;
          */

          // So for now we only do this in migration.
          //
          if (dropped_ == 0)
          {
            if (fk.not_deferrable ())
              pre_statement ();
            else
            {
              if (format_ != schema_format::sql)
                return;

              os << "/*" << endl;
            }

            os << "ALTER TABLE " << quote_id (t.name ()) << endl
               << "  DROP FOREIGN KEY " << quote_id (fk.name ()) << endl;

            if (fk.not_deferrable ())
              post_statement ();
            else
              os << "*/" << endl
                 << endl;
          }
        }

        using base::drop;

        virtual void
        traverse (sema_rel::drop_foreign_key& dfk)
        {
          // Find the foreign key we are dropping in the base model.
          //
          sema_rel::foreign_key& fk (find<sema_rel::foreign_key> (dfk));

          if (fk.not_deferrable () || in_comment)
            base::traverse (dfk);
          else
          {
            if (format_ != schema_format::sql)
              return;

            os << endl
               << "  /*"
               << endl;

            drop (dfk);

            os << endl
               << "  */";
          }
        }

        virtual void
        drop_header ()
        {
          os << "DROP FOREIGN KEY ";
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

      //
      // Create.
      //

      struct create_column: relational::create_column, context
      {
        create_column (base const& x): base (x) {}

        virtual void
        auto_ (sema_rel::primary_key&)
        {
          os << " AUTO_INCREMENT";
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
              "ON DELETE clause but is disabled in MySQL due to lack "
              "of deferrable constraint support" << endl;

            cerr << "info: consider using non-deferrable foreign keys (" <<
              "--fkeys-deferrable-mode)" << endl;
          }
        }

        virtual void
        traverse_create (sema_rel::foreign_key& fk)
        {
          // MySQL does not support deferrable constraint checking. Output
          // such foreign keys as comments, for documentation, unless we
          // are generating embedded schema.
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
          if (fk.not_deferrable () || in_comment)
            base::traverse_add (fk);
          else
          {
            diagnose (fk);

            if (format_ != schema_format::sql)
              return;

            os << endl
               << "  /*"
               << endl;

            add (fk);

            os << endl
               << "  */";
          }
        }

        virtual void
        deferrable (sema_rel::deferrable)
        {
          // This will still be called to output the comment.
        }
      };
      entry<create_foreign_key> create_foreign_key_;

      struct create_index: relational::create_index, context
      {
        create_index (base const& x): base (x) {}

        virtual void
        create (sema_rel::index& in)
        {
          os << "CREATE ";

          if (!in.type ().empty ())
            os << in.type () << ' ';

          os << "INDEX " << name (in);

          if (!in.method ().empty ())
            os << " USING " << in.method ();

          os << endl
             << "  ON " << table_name (in) << " (";

          columns (in);

          os << ")" << endl;

          if (!in.options ().empty ())
            os << ' ' << in.options () << endl;
        }
      };
      entry<create_index> create_index_;

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
              if (!fk->count ("mysql-fk-defined") &&
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

                os << "ALTER TABLE " << quote_id (t.name ());

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
        alter_header ()
        {
          os << "MODIFY COLUMN ";
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
          if (check<sema_rel::add_column> (at) ||
              check_alter_column_null (at, true))
            return false;

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
          if (check_drop_deferrable_only (at))
          {
            if (format_ != schema_format::sql)
              return;

            os << "/*" << endl;
            in_comment = true;

            os << "ALTER TABLE " << quote_id (at.name ());
            instance<drop_foreign_key> dfk (*this);
            trav_rel::unames n (*dfk);
            names (at, n);
            os << endl;

            in_comment = false;
            os << "*/" << endl
               << endl;
          }
          else
            base::alter (at);
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
          if (check<sema_rel::drop_column> (at) ||
              check_alter_column_null (at, false))
            return false;

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
          if (check_add_deferrable_only (at))
          {
            if (format_ != schema_format::sql)
              return;

            os << "/*" << endl;
            in_comment = true;

            os << "ALTER TABLE " << quote_id (at.name ());
            instance<create_foreign_key> cfk (*this);
            trav_rel::unames n (*cfk);
            names (at, n);
            os << endl;

            in_comment = false;
            os << "*/" << endl
               << endl;
          }
          else
            base::alter (at);
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
             << "  " << qn_ << " VARCHAR(128) NOT NULL PRIMARY KEY," << endl
             << "  " << qv_ << " BIGINT UNSIGNED NOT NULL," << endl
             << "  " << qm_ << " TINYINT(1) NOT NULL)" << endl;

          string const& engine (options.mysql_engine ());
          if (engine != "default")
            os << " ENGINE=" << engine << endl;

          post_statement ();
        }

        virtual void
        create (sema_rel::version v)
        {
          pre_statement ();

          os << "INSERT IGNORE INTO " << qt_ << " (" << endl
             << "  " << qn_ << ", " << qv_ << ", " << qm_ << ")" << endl
             << "  VALUES (" << qs_ << ", " << v << ", 0)" << endl;

          post_statement ();
        }
      };
      entry<version_table> version_table_;
    }
  }
}
