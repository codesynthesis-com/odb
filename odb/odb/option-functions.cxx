// file      : odb/option-functions.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <set>
#include <utility> // std::make_pair()

#include <odb/option-functions.hxx>

using namespace std;

void
process_options (options& o)
{
  database db (o.database ()[0]);

  // If --generate-schema-only was specified, then set --generate-schema
  // as well.
  //
  if (o.generate_schema_only ())
    o.generate_schema (true);

  // If --warn-hard was specified, then set both --warn-hard-{add,delete}.
  //
  if (o.warn_hard ())
  {
    o.warn_hard_add (true);
    o.warn_hard_delete (true);
  }

  // Set the default schema format depending on the database.
  //
  if (o.generate_schema () && o.schema_format ()[db].empty ())
  {
    set<schema_format>& f (o.schema_format ()[db]);

    switch (db)
    {
    case database::common:
      {
        break; // No schema for common.
      }
    case database::mssql:
    case database::mysql:
    case database::oracle:
    case database::pgsql:
      {
        f.insert (schema_format::sql);
        break;
      }
    case database::sqlite:
      {
        f.insert (schema_format::embedded);
        break;
      }
    }
  }

  // Set default --schema-version-table value.
  //
  if (o.schema_version_table ().count (db) == 0)
    o.schema_version_table ()[db] = "schema_version";

  // Set default --schema-name value.
  //
  if (o.schema_name ().count (db) == 0)
    o.schema_name ()[db] = "";

  // Set default --fkeys-deferrable-mode value.
  //
  if (o.fkeys_deferrable_mode ().count (db) == 0)
    o.fkeys_deferrable_mode ()[db] = deferrable::deferred;

  // Set default --{export,extern}-symbol values.
  //
  if (o.export_symbol ().count (db) == 0)
    o.export_symbol ()[db] = "";

  if (o.extern_symbol ().count (db) == 0)
    o.extern_symbol ()[db] = "";

  // Set default --*-file-suffix values.
  //
  {
    database cm (database::common);

    o.odb_file_suffix ().insert (make_pair (cm, string ("-odb")));
    o.sql_file_suffix ().insert (make_pair (cm, string ("")));
    o.schema_file_suffix ().insert (make_pair (cm, string ("-schema")));
    o.changelog_file_suffix ().insert (make_pair (cm, string ("")));
  }

  if (o.multi_database () == multi_database::disabled)
  {
    o.odb_file_suffix ().insert (make_pair (db, string ("-odb")));
    o.sql_file_suffix ().insert (make_pair (db, string ("")));
    o.schema_file_suffix ().insert (make_pair (db, string ("-schema")));
    o.changelog_file_suffix ().insert (make_pair (db, string ("")));
  }
  else
  {
    o.odb_file_suffix ().insert (make_pair (db, "-odb-" + db.string ()));
    o.sql_file_suffix ().insert (make_pair (db, "-" + db.string ()));
    o.schema_file_suffix ().insert (make_pair (db, "-schema-" + db.string ()));
    o.changelog_file_suffix ().insert (make_pair (db, '-' + db.string ()));
  }

  // Set default --default-database value.
  //
  if (!o.default_database_specified ())
  {
    switch (o.multi_database ())
    {
    case multi_database::disabled:
      {
        o.default_database (db);
        o.default_database_specified (true);
        break;
      }
    case multi_database::dynamic:
      {
        o.default_database (database::common);
        o.default_database_specified (true);
        break;
      }
    case multi_database::static_:
      {
        // No default database unless explicitly specified.
        break;
      }
    }
  }
}
