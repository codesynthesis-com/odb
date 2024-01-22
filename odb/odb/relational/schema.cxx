// file      : odb/relational/schema.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>
#include <limits>
#include <sstream>

#include <odb/relational/schema.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

namespace relational
{
  namespace schema
  {
    void
    generate_prologue ()
    {
      instance<sql_file> file;
      file->prologue ();
    }

    void
    generate_epilogue ()
    {
      instance<sql_file> file;
      file->epilogue ();
    }

    void
    generate_drop ()
    {
      context ctx;
      instance<sql_emitter> em;
      emitter_ostream emos (*em);

      schema_format f (schema_format::sql);

      instance<drop_model> model (*em, emos, f);
      instance<drop_table> table (*em, emos, f);
      trav_rel::qnames names;

      model >> names >> table;

      // Pass 1 and 2.
      //
      for (unsigned short pass (1); pass < 3; ++pass)
      {
        model->pass (pass);
        table->pass (pass);

        model->traverse (*ctx.model);
      }

      if (ctx.model->version () != 0 &&
          !ctx.options.suppress_schema_version ())
      {
        instance<version_table> vt (*em, emos, f);
        vt->create_table ();
        vt->drop ();
      }
    }

    void
    generate_create ()
    {
      context ctx;
      instance<sql_emitter> em;
      emitter_ostream emos (*em);

      schema_format f (schema_format::sql);

      instance<create_model> model (*em, emos, f);
      instance<create_table> table (*em, emos, f);
      trav_rel::qnames names;

      model >> names >> table;

      // Pass 1 and 2.
      //
      for (unsigned short pass (1); pass < 3; ++pass)
      {
        model->pass (pass);
        table->pass (pass);

        model->traverse (*ctx.model);
      }

      if (ctx.model->version () != 0 &&
          !ctx.options.suppress_schema_version ())
      {
        instance<version_table> vt (*em, emos, f);

        if (ctx.options.omit_drop ())
          vt->create_table ();

        vt->create (ctx.model->version ());
      }
    }

    void
    generate_migrate_pre (sema_rel::changeset& cs)
    {
      context ctx;
      instance<sql_emitter> em;
      emitter_ostream emos (*em);

      schema_format f (schema_format::sql);

      instance<changeset_pre> changeset (*em, emos, f);
      instance<create_table> ctable (*em, emos, f);
      instance<alter_table_pre> atable (*em, emos, f);
      trav_rel::qnames names;

      changeset >> names;
      names >> ctable;
      names >> atable;

      // Pass 1 and 2.
      //
      for (unsigned short pass (1); pass < 3; ++pass)
      {
        changeset->pass (pass);
        ctable->pass (pass);
        atable->pass (pass);

        changeset->traverse (cs);
      }

      if (!ctx.options.suppress_schema_version ())
      {
        instance<version_table> vt (*em, emos, f);
        vt->migrate_pre (cs.version ());
      }
    }

    void
    generate_migrate_post (sema_rel::changeset& cs)
    {
      context ctx;
      instance<sql_emitter> em;
      emitter_ostream emos (*em);

      schema_format f (schema_format::sql);

      instance<changeset_post> changeset (*em, emos, f);
      instance<drop_table> dtable (*em, emos, f);
      instance<alter_table_post> atable (*em, emos, f);
      trav_rel::qnames names;

      changeset >> names;
      names >> dtable;
      names >> atable;

      // Pass 1 and 2.
      //
      for (unsigned short pass (1); pass < 3; ++pass)
      {
        changeset->pass (pass);
        dtable->pass (pass);
        atable->pass (pass);

        changeset->traverse (cs);
      }

      if (!ctx.options.suppress_schema_version ())
      {
        instance<version_table> vt (*em, emos, f);
        vt->migrate_post ();
      }
    }
  }
}
