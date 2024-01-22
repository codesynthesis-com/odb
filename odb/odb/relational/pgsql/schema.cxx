// file      : odb/relational/pgsql/schema.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/schema.hxx>

#include <odb/relational/pgsql/common.hxx>
#include <odb/relational/pgsql/context.hxx>

using namespace std;

namespace relational
{
  namespace pgsql
  {
    namespace schema
    {
      namespace relational = relational::schema;
      using relational::table_set;

      //
      // Drop.
      //

      struct drop_table: relational::drop_table, context
      {
        drop_table (base const& x): base (x) {}

        virtual void
        traverse (sema_rel::table& t, bool migration)
        {
          // For migration drop foreign keys explicitly in pre-migration.
          //
          if (migration)
          {
            base::traverse (t, migration);
            return;
          }

          // For schema creation we use the CASCADE clause to drop foreign
          // keys.
          //
          if (pass_ != 2)
            return;

          pre_statement ();
          os << "DROP TABLE " << (migration ? "" : "IF EXISTS ") <<
            quote_id (t.name ()) << " CASCADE" << endl;
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
        type (sema_rel::column& c, bool auto_)
        {
          if (auto_)
          {
            // This should never fail since we have already parsed this.
            //
            sql_type const& t (parse_sql_type (c.type ()));

            // The model creation code makes sure it is one of these type.
            //
            if (t.type == sql_type::INTEGER)
              os << "SERIAL";
            else if (t.type == sql_type::BIGINT)
              os << "BIGSERIAL";
          }
          else
            base::type (c, auto_);
        }
      };
      entry<create_column> create_column_;

      struct create_foreign_key: relational::create_foreign_key, context
      {
        create_foreign_key (base const& x): base (x) {}

        virtual void
        deferrable (sema_rel::deferrable d)
        {
          os << endl
             << "    INITIALLY " << d;
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
          {
            // Handle the CONCURRENTLY keyword.
            //
            string const& t (in.type ());

            if (t == "concurrently" || t == "CONCURRENTLY")
            {
              os << "INDEX " << t;
            }
            else
            {
              size_t p (t.rfind (' '));
              string s (t, (p != string::npos ? p + 1 : 0), string::npos);

              if (s == "concurrently" || s == "CONCURRENTLY")
                os << string (t, 0, p) << " INDEX " << s;
              else
                os << t << " INDEX";
            }
          }
          else
            os << "INDEX";

          os << " " << name (in) << endl
             << "  ON " << table_name (in);

          if (!in.method ().empty ())
            os << " USING " << in.method ();

          os << " (";
          columns (in);
          os << ")" << endl;

          if (!in.options ().empty ())
            os << ' ' << in.options () << endl;
        }
      };
      entry<create_index> create_index_;

      //
      // Alter.
      //

      struct alter_column: relational::alter_column, context
      {
        alter_column (base const& x): base (x) {}

        virtual void
        alter (sema_rel::column& c)
        {
          os << quote_id (c.name ()) << " " <<
            (c.null () ? "DROP" : "SET") << " NOT NULL";
        }
      };
      entry<alter_column> alter_column_;

      //
      // Schema version table.
      //

      struct version_table: relational::version_table, context
      {
        version_table (base const& x): base (x) {}

        // PostgreSQL prior to 9.1 doesn't support IF NOT EXISTS in
        // CREATE TABLE. We also cannot use IF-ELSE construct in plain
        // SQL. To make it at least work for a single schema, we are
        // going to drop the schema version table after the DROP
        // statements and then unconditionally create it after CREATE.
        //
        virtual void
        create_table ()
        {
          if (options.pgsql_server_version () >= pgsql_version (9, 1))
          {
            pre_statement ();

            os << "CREATE TABLE IF NOT EXISTS " << qt_ << " (" << endl
               << "  " << qn_ << " TEXT NOT NULL PRIMARY KEY," << endl
               << "  " << qv_ << " BIGINT NOT NULL," << endl
               << "  " << qm_ << " BOOLEAN NOT NULL)" << endl;

            post_statement ();
          }
        }

        virtual void
        drop ()
        {
          pre_statement ();

          if (options.pgsql_server_version () >= pgsql_version (9, 1))
            os << "DELETE FROM " << qt_ << endl
               << "  WHERE " << qn_ << " = " << qs_ << endl;
          else
            os << "DROP TABLE IF EXISTS " << qt_ << endl;

          post_statement ();
        }

        virtual void
        create (sema_rel::version v)
        {
          pre_statement ();

          if (options.pgsql_server_version () >= pgsql_version (9, 1))
          {
            os << "INSERT INTO " << qt_ << " (" << endl
               << "  " << qn_ << ", " << qv_ << ", " << qm_ << ")" << endl
               << "  SELECT " << qs_ << ", " << v << ", FALSE" << endl
               << "  WHERE NOT EXISTS (" << endl
               << "    SELECT 1 FROM " << qt_ << " WHERE " << qn_ << " = " <<
              qs_ << ")" << endl;
          }
          else
          {
            os << "CREATE TABLE " << qt_ << " (" << endl
               << "  " << qn_ << " TEXT NOT NULL PRIMARY KEY," << endl
               << "  " << qv_ << " BIGINT NOT NULL," << endl
               << "  " << qm_ << " BOOLEAN NOT NULL)" << endl;

            post_statement ();
            pre_statement ();

            os << "INSERT INTO " << qt_ << " (" << endl
               << "  " << qn_ << ", " << qv_ << ", " << qm_ << ")" << endl
               << "  VALUES (" << qs_ << ", " << v << ", FALSE)" << endl;
          }

          post_statement ();
        }

        virtual void
        migrate_pre (sema_rel::version v)
        {
          pre_statement ();

          os << "UPDATE " << qt_ << endl
             << "  SET " << qv_ << " = " << v << ", " << qm_ << " = TRUE" << endl
             << "  WHERE " << qn_ << " = " << qs_ << endl;

          post_statement ();
        }

        virtual void
        migrate_post ()
        {
          pre_statement ();

          os << "UPDATE " << qt_ << endl
             << "  SET " << qm_ << " = FALSE" << endl
             << "  WHERE " << qn_ << " = " << qs_ << endl;

          post_statement ();
        }

      };
      entry<version_table> version_table_;
    }
  }
}
