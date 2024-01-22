// file      : odb/relational/schema-source.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/schema-source.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

namespace relational
{
  namespace schema
  {
    void
    generate_source (sema_rel::changelog* log)
    {
      context ctx;
      ostream& os (ctx.os);
      database db (ctx.db);
      options const& ops (ctx.options);
      sema_rel::model& model (*ctx.model);
      string const& schema_name (ops.schema_name ()[db]);

      if (log != 0 && ops.suppress_migration ())
        log = 0;

      bool schema_version (
        model.version () != 0 && !ctx.options.suppress_schema_version ());

      instance<cxx_emitter> emitter;
      emitter_ostream emitter_os (*emitter);
      schema_format format (schema_format::embedded);

      if (!model.names_empty () || log != 0 || schema_version)
        os << "namespace odb"
           << "{";

      // Schema.
      //
      if (!model.names_empty () || schema_version)
      {
        os << "static bool" << endl
           << "create_schema (database& db, unsigned short pass, bool drop)"
           << "{"
           << "ODB_POTENTIALLY_UNUSED (db);"
           << "ODB_POTENTIALLY_UNUSED (pass);"
           << "ODB_POTENTIALLY_UNUSED (drop);"
           << endl;

        // Drop.
        //
        if (!ops.omit_drop ())
        {
          bool close (false);

          os << "if (drop)"
             << "{";

          instance<drop_model> dmodel (*emitter, emitter_os, format);
          instance<drop_table> dtable (*emitter, emitter_os, format);
          trav_rel::qnames names;
          dmodel >> names >> dtable;

          for (unsigned short pass (1); pass < 3; ++pass)
          {
            emitter->pass (pass);
            dmodel->pass (pass);
            dtable->pass (pass);

            dmodel->traverse (model);

            close = close || !emitter->empty ();
          }

          if (schema_version)
          {
            instance<version_table> vt (*emitter, emitter_os, format);
            vt->create_table ();
            vt->drop ();
            close = true;
          }

          if (close) // Close the last case and the switch block.
            os << "return false;"
               << "}"  // case
               << "}";  // switch

          os << "}";
        }

        // Create.
        //
        if (!ops.omit_create ())
        {
          bool close (false);

          if (ops.omit_drop ())
            os << "if (!drop)";
          else
            os << "else";

          os << "{";

          instance<create_model> cmodel (*emitter, emitter_os, format);
          instance<create_table> ctable (*emitter, emitter_os, format);
          trav_rel::qnames names;
          cmodel >> names >> ctable;

          for (unsigned short pass (1); pass < 3; ++pass)
          {
            emitter->pass (pass);
            cmodel->pass (pass);
            ctable->pass (pass);

            cmodel->traverse (model);

            close = close || !emitter->empty ();
          }

          if (schema_version)
          {
            instance<version_table> vt (*emitter, emitter_os, format);
            vt->create_table ();
            vt->create (model.version ());
            close = true;
          }

          if (close) // Close the last case and the switch block.
            os << "return false;"
               << "}"  // case
               << "}"; // switch

          os << "}";
        }

        os << "return false;"
           << "}";

        os << "static const schema_catalog_create_entry" << endl
           << "create_schema_entry_ (" << endl
           << "id_" << db << "," << endl
           << context::strlit (schema_name) << "," << endl
           << "&create_schema);"
           << endl;
      }

      // Migration.
      //
      if (log != 0)
      {
        // Create NULL migration entry for the base version so that we
        // get the complete version range (base, current) at runtime.
        // Code in schema_catalog relies on this.
        //
        os << "static const schema_catalog_migrate_entry" << endl
           << "migrate_schema_entry_" << log->model ().version () <<
          "_ (" << endl
           << "id_" << db << "," << endl
           << context::strlit (schema_name) << "," << endl
           << log->model ().version () << "ULL," << endl
           << "0);"
           << endl;

        for (sema_rel::changelog::contains_changeset_iterator i (
               log->contains_changeset_begin ());
             i != log->contains_changeset_end (); ++i)
        {
          sema_rel::changeset& cs (i->changeset ());

          os << "static bool" << endl
             << "migrate_schema_" << cs.version () << " (database& db, " <<
            "unsigned short pass, bool pre)"
             << "{"
             << "ODB_POTENTIALLY_UNUSED (db);"
             << "ODB_POTENTIALLY_UNUSED (pass);"
             << "ODB_POTENTIALLY_UNUSED (pre);"
             << endl;

          // Pre.
          //
          {
            bool close (false);

            os << "if (pre)"
               << "{";

            instance<changeset_pre> changeset (*emitter, emitter_os, format);
            instance<create_table> ctable (*emitter, emitter_os, format);
            instance<alter_table_pre> atable (*emitter, emitter_os, format);
            trav_rel::qnames names;
            changeset >> names;
            names >> ctable;
            names >> atable;

            for (unsigned short pass (1); pass < 3; ++pass)
            {
              emitter->pass (pass);
              changeset->pass (pass);
              ctable->pass (pass);
              atable->pass (pass);

              changeset->traverse (cs);

              close = close || !emitter->empty ();
            }

            if (!ctx.options.suppress_schema_version ())
            {
              instance<version_table> vt (*emitter, emitter_os, format);
              vt->migrate_pre (cs.version ());
              close = true;
            }

            if (close) // Close the last case and the switch block.
              os << "return false;"
                 << "}"  // case
                 << "}";  // switch

            os << "}";
          }

          // Post.
          //
          {
            bool close (false);

            os << "else"
               << "{";

            instance<changeset_post> changeset (*emitter, emitter_os, format);
            instance<drop_table> dtable (*emitter, emitter_os, format);
            instance<alter_table_post> atable (*emitter, emitter_os, format);
            trav_rel::qnames names;
            changeset >> names;
            names >> dtable;
            names >> atable;

            for (unsigned short pass (1); pass < 3; ++pass)
            {
              emitter->pass (pass);
              changeset->pass (pass);
              dtable->pass (pass);
              atable->pass (pass);

              changeset->traverse (cs);

              close = close || !emitter->empty ();
            }

            if (!ctx.options.suppress_schema_version ())
            {
              instance<version_table> vt (*emitter, emitter_os, format);
              vt->migrate_post ();
              close = true;
            }

            if (close) // Close the last case and the switch block.
              os << "return false;"
                 << "}"  // case
                 << "}"; // switch

            os << "}";
          }

          os << "return false;"
             << "}";

          os << "static const schema_catalog_migrate_entry" << endl
             << "migrate_schema_entry_" << cs.version () << "_ (" << endl
             << "id_" << db << "," << endl
             << context::strlit (schema_name) << "," << endl
             << cs.version () << "ULL," << endl
             << "&migrate_schema_" << cs.version () << ");"
             << endl;
        }
      }

      if (!model.names_empty () || log != 0 || schema_version)
        os << "}"; // namespace odb
    }
  }
}
