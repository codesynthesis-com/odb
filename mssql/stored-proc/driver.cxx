// file      : mssql/stored-proc/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test SQL Server stored procedure support.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace mssql = odb::mssql;
using namespace mssql;

void
create_procedure (database& db, const string& name, const string& body)
{
  transaction t (db.begin ());

  string s (db.query_value<default_schema> ().name);

  db.execute (
    "IF EXISTS ("
    "  SELECT * FROM sysobjects"
    "    WHERE name = '" + name + "' AND user_name(uid) = '" + s +"')"
    "  DROP PROCEDURE [" + s + "].[" + name + "]");

  db.execute ("CREATE PROCEDURE [" + s + "].[" + name + "] " + body);

  t.commit ();
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    object o1 (1, "a");
    object o2 (2, "b");
    object o3 (3, "c");

    {
      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      db->persist (o3);
      t.commit ();
    }

    {
      create_procedure (
        *db, "select_all_objects",
        "AS"
        "  SELECT num, str FROM mssql_stored_proc_object ORDER BY id;");

      typedef odb::result<select_all_objects> result;

      transaction t (db->begin ());

      result r (db->query<select_all_objects> ());

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->num << " " << i->str << endl;
      cout << endl;

      t.commit ();
    }

    {
      create_procedure (
        *db, "select_objects",
        "(@id INT, @n VARCHAR(512))"
        "AS"
        "  SELECT str FROM mssql_stored_proc_object "
        "    WHERE [id] = @id OR [num] = @n ORDER BY id;");

      typedef mssql::query<select_objects> query;
      typedef odb::result<select_objects> result;

      transaction t (db->begin ());

      result r (db->query<select_objects> (
                  query::_val (o1.id) + "," + query::_val (o2.num)));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->str << endl;
      cout << endl;

      t.commit ();
    }

    {
      create_procedure (
        *db, "objects_min_max",
        "(@min INT = NULL OUTPUT, @max INT = NULL OUTPUT)"
        "AS"
        "  SELECT @min = MIN(num), @max = MAX(num)"
        "    FROM mssql_stored_proc_object;");

      create_procedure (
        *db, "objects_min_max_odb",
        "AS"
        "  DECLARE @min INT, @max INT;"
        "  EXEC objects_min_max @min OUTPUT, @max OUTPUT;"
        "  SELECT @min, @max;");

      transaction t (db->begin ());

      objects_min_max omm (db->query_value<objects_min_max> ());
      cout << omm.num_min << " " << omm.num_max << endl
           << endl;

      t.commit ();
    }

    {
      create_procedure (
        *db, "insert_object_id",
        "(@n INT, @s VARCHAR(512))"
        "AS"
        "  INSERT INTO mssql_stored_proc_object([num], [str])"
        "    VALUES(@n, @s);");

      {
        typedef mssql::query<insert_object> query;

        transaction t (db->begin ());

        db->query_one<insert_object> (
          query::_val (4) + "," + query::_val ("d"));

        auto_ptr<object> o (db->load<object> (4));
        cout << o->num << " " << o->str << endl
             << endl;

        t.commit ();
      }

      {
        typedef mssql::query<no_result> query;

        transaction t (db->begin ());

        db->query_one<no_result> (
          "EXEC insert_object_id" + query::_val (5) + "," + query::_val ("e"));

        auto_ptr<object> o (db->load<object> (5));
        cout << o->num << " " << o->str << endl
             << endl;

        t.commit ();
      }
    }

    {
      create_procedure (
        *db, "insert_object_id",
        "(@n INT, @s VARCHAR(512), @id INT = NULL OUTPUT)"
        "AS"
        "  INSERT INTO mssql_stored_proc_object([num], [str])"
        "    VALUES(@n, @s);"
        "  SET @id = SCOPE_IDENTITY();"
        "  RETURN 123;");

      typedef mssql::query<insert_object_id> query;

      {
        create_procedure (
          *db, "insert_object_id_odb",
          "(@n INT, @s VARCHAR(512))"
          "AS"
          "  DECLARE @id INT;"
          "  DECLARE @ret INT;"
          "  EXEC @ret = insert_object_id @n, @s, @id OUTPUT;"
          "  SELECT @ret, @id;");

        transaction t (db->begin ());

        insert_object_id io (
          db->query_value<insert_object_id> (
            query::_val (6) + "," + query::_val ("f")));

        cout << io.ret << " " << io.id << endl
             << endl;

        t.commit ();
      }

      // An alternative implementation that produces a different
      // result set configuration at the ODBC level.
      //
      {
        create_procedure (
          *db, "insert_object_id_odb",
          "(@n INT, @s VARCHAR(512))"
          "AS"
          "  DECLARE @id INT;"
          "  DECLARE @ret INT;"
          "  DECLARE @tbl TABLE(dummy INT);"
          "  INSERT INTO @tbl EXEC @ret = insert_object_id @n, @s, @id OUTPUT;"
          "  SELECT @ret, @id;");

        transaction t (db->begin ());

        insert_object_id io (
          db->query_value<insert_object_id> (
            query::_val (7) + "," + query::_val ("g")));

        cout << io.ret << " " << io.id << endl
             << endl;

        t.commit ();
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
