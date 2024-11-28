// file      : pgsql/stored-proc/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test PostgreSQL stored procedure and function support.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

static void
create_function (database& db, const string& signature, const string& body)
{
  transaction t (db.begin ());
  db.execute ("DROP ROUTINE IF EXISTS " + signature);
  db.execute ("CREATE FUNCTION " + signature + ' ' + body);
  t.commit ();
}

static void
create_procedure (database& db, const string& signature, const string& body)
{
  transaction t (db.begin ());
  db.execute ("DROP ROUTINE IF EXISTS " + signature);
  db.execute ("CREATE PROCEDURE " + signature + ' ' + body);
  t.commit ();
}

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_specific_database<database> (argc, argv));

    person john (1, "John", "Doe",  33);
    person jane (2, "Jane", "Doe",  32);
    person joe  (3, "Joe",  "Dirt", 30);

    {
      transaction t (db->begin ());
      db->persist (john);
      db->persist (jane);
      db->persist (joe);
      t.commit ();
    }

    // Stored functions.
    //
    // Notes (as per the PostgreSQL documentation):
    //
    // - When there are OUT or INOUT parameters, the RETURNS clause can be
    //   omitted. If present, it must agree with the result type implied by
    //   the output parameters: RECORD if there are multiple output
    //   parameters, or the same type as the single output parameter.
    //
    // - Also, OUT and INOUT arguments cannot be used together with the
    //   RETURNS TABLE notation.
    //

    // Return rowset.
    //
    {
      // Note that if use `IN min_age SMALLINT, IN max_age SMALLINT` here,
      // then we end up with the following runtime error, unless passing the
      // `unsigned short` type values to the query::_val() calls:
      //
      // 42883: ERROR:  function person_range(integer, integer) does not exist
      // LINE 1: SELECT * FROM person_range ($1, $2)
      //                       ^
      // HINT:  No function matches the given name and argument types. You
      //        might need to add explicit type casts.
      //
      // Note that the PostgreSQL's INTEGER type for the function parameters
      // works well for `unsigned short` C++ type values.
      //
      create_function (*db,
                       "person_range (IN min_age INTEGER,\n"
                                     "IN max_age INTEGER,\n"
                                     "OUT age SMALLINT,\n"
                                     "OUT first TEXT,\n"
                                     "OUT last TEXT)",
                       "RETURNS SETOF RECORD AS $$\n"
                       "  SELECT age, first, last\n"
                       "    FROM pgsql_stored_proc_person\n"
                       "    WHERE age >= min_age AND age <= max_age\n"
                       "    ORDER BY id;\n"
                       "$$ LANGUAGE SQL STABLE;");

      typedef odb::query<person_range> query;
      typedef odb::result<person_range> result;

      transaction t (db->begin ());

      // Return multiple rows.
      //
      {
        result r (
          db->query<person_range> (
            query::_val (1) + "," + query::_val (32)));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first << " " << i->last << " " << i->age << endl;

        cout << endl;
      }

      // Return empty rowset.
      //
      {
        result r (
          db->query<person_range> (
            query::_val (1) + "," + query::_val (10)));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first << " " << i->last << " " << i->age << endl;

        cout << endl;
      }

      t.commit ();
    }

    // As above (return rowset) but using the alternative `RETURNS TABLE`
    // notation.
    //
    {
      create_function (*db,
                       "person_range (IN min_age INTEGER,\n"
                                     "IN max_age INTEGER)",
                       "RETURNS TABLE(age SMALLINT,\n"
                                     "first TEXT,\n"
                                     "last TEXT) AS $$\n"
                       "  SELECT age, first, last\n"
                       "    FROM pgsql_stored_proc_person\n"
                       "    WHERE age >= min_age AND age <= max_age\n"
                       "    ORDER BY id;\n"
                       "$$ LANGUAGE SQL STABLE;");

      typedef odb::query<person_range> query;
      typedef odb::result<person_range> result;

      transaction t (db->begin ());

      result r (
        db->query<person_range> (
          query::_val (1) + "," + query::_val (32)));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->first << " " << i->last << " " << i->age << endl;

      cout << endl;

      t.commit ();
    }

    // Similar to the above but using INOUT parameter.
    //
    {
      create_function (*db,
                       "person_range (INOUT person_age INTEGER,\n"
                                     "OUT first TEXT,\n"
                                     "OUT last TEXT)",
                       "RETURNS SETOF RECORD AS $$\n"
                       "  SELECT age, first, last\n"
                       "    FROM pgsql_stored_proc_person\n"
                       "    WHERE age = person_age\n"
                       "    ORDER BY id;\n"
                       "$$ LANGUAGE SQL STABLE;");

      typedef odb::query<person_range> query;
      typedef odb::result<person_range> result;

      transaction t (db->begin ());

      // Return multiple rows.
      //
      {
        result r (
          db->query<person_range> (query (query::_val (32))));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first << " " << i->last << " " << i->age << endl;

        cout << endl;
      }

      t.commit ();
    }

    // Return single row (no SETOF).
    //
    // Note: always returns one row, regardless of what is returned by SELECT
    // (see below for details).
    //
    {
      create_function (*db,
                       "person_range (IN min_age INTEGER,\n"
                                     "IN max_age INTEGER,\n"
                                     "OUT age SMALLINT,\n"
                                     "OUT first TEXT,\n"
                                     "OUT last TEXT)",
                       "AS $$\n"
                       "  SELECT age, first, last\n"
                       "    FROM pgsql_stored_proc_person\n"
                       "    WHERE age >= min_age AND age <= max_age\n"
                       "    ORDER BY id;\n"
                       "$$ LANGUAGE SQL STABLE;");

      typedef odb::query<person_range> query;

      transaction t (db->begin ());

      // Return the first row of the multiple ones generated by the SELECT
      // statement in the function implementation.
      //
      {
        person_range r (
          db->query_value<person_range> (
            query::_val (1) + "," + query::_val (32)));

        cout << r.first << " " << r.last << " " << r.age << endl
             << endl;
      }

      // Return the single row with the NULL column values if the SELECT
      // statement in the function implementation ends up with an empty
      // rowset.
      //
      {
        person_range r (
          db->query_value<person_range> (
            query::_val (1) + "," + query::_val (10)));

        cout << r.first << " " << r.last << " " << r.age << endl
             << endl;
      }

      t.commit ();
    }

    // Similar to the above but use INOUT parameter.
    //
    {
      create_function (*db,
                       "person_range (INOUT person_age INTEGER,\n"
                                     "OUT first TEXT,\n"
                                     "OUT last TEXT)",
                       "AS $$\n"
                       "  SELECT age, first, last\n"
                       "    FROM pgsql_stored_proc_person\n"
                       "    WHERE age = person_age\n"
                       "    ORDER BY id;\n"
                       "$$ LANGUAGE SQL STABLE;");

      typedef odb::query<person_range> query;

      transaction t (db->begin ());

      // Return the first row of the multiple ones.
      //
      {
        person_range r (
          db->query_value<person_range> (query (query::_val (32))));

        cout << r.first << " " << r.last << " " << r.age << endl
             << endl;
      }

      t.commit ();
    }

    // No arguments.
    //
    {
      create_function (*db,
                       "person_count ()",
                       "RETURNS BIGINT AS $$\n"
                       "  SELECT count(age)\n"
                       "    FROM pgsql_stored_proc_person;\n"
                       "$$ LANGUAGE SQL STABLE;");

      transaction t (db->begin ());

      person_count r (db->query_value<person_count> ());

      cout << r.count << endl
           << endl;

      t.commit ();
    }

    // Modify the table and return void.
    //
    // Note that in reality the function returns a single row with a single
    // column of type void containing the NULL value.
    //
    {
      create_function (*db,
                       "person_age_increment (IN min_age INTEGER,\n"
                                             "IN max_age INTEGER)",
                       "RETURNS VOID AS $$\n"
                       "  UPDATE pgsql_stored_proc_person\n"
                       "    SET age = age + 1\n"
                       "    WHERE age >= min_age AND age <= max_age;\n"
                       "$$ LANGUAGE SQL;");

      typedef odb::query<person_age_increment> query;
      typedef odb::result<person> result;

      transaction t (db->begin ());

      {
        unique_ptr<person_age_increment> fr (
          db->query_one<person_age_increment> (
            query::_val (1) + "," + query::_val (32)));

        assert (fr != nullptr);

        result r (db->query<person> ("ORDER BY id"));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first () << " " << i->last () << " " << i->age () << endl;

        cout << endl;
      }

      t.commit ();
    }

    // As above but also use the SETOF specifier in the function definition.
    // In this case the function returns an empty rowset.
    //
    {
      create_function (*db,
                       "person_age_increment (IN min_age INTEGER,\n"
                                             "IN max_age INTEGER)",
                       "RETURNS SETOF VOID AS $$\n"
                       "  UPDATE pgsql_stored_proc_person\n"
                       "    SET age = age + 1\n"
                       "    WHERE age >= min_age AND age <= max_age;\n"
                       "$$ LANGUAGE SQL;");

      typedef odb::query<person_age_increment> query;
      typedef odb::result<person> result;

      transaction t (db->begin ());

      {
        unique_ptr<person_age_increment> fr (
          db->query_one<person_age_increment> (
            query::_val (1) + "," + query::_val (32)));

        assert (fr == nullptr);

        result r (db->query<person> ("ORDER BY id"));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first () << " " << i->last () << " " << i->age () << endl;

        cout << endl;
      }

      t.commit ();
    }

    // Stored procedures.
    //
    // Notes (as per the PostgreSQL documentation):
    //
    // - Output parameters are also supported in procedures, but they work a
    //   bit differently compared to functions. In the CALL statement, output
    //   parameters must be included in the argument list, customarily as
    //   NULL. They are returned as a single row.
    //
    // Note that we could automatically add the NULL values for all the output
    // arguments since their number should match the number of members in the
    // structure associated with the view. But we would need to make sure the
    // user didn't do that. Plus they could be mixed with IN. Plus there could
    // be INOUT.

    // Assign single row columns to the output parameters.
    //
    // Note that if there are output parameters specified, then the stored
    // procedure assigns the columns of the first row produced by the
    // underlying statement(s) to the output parameters. If the function
    // implementation produces no rows then this is a libpq API level error.
    //
    // Also note that stored procedures cannot return rowsets.
    //
    {
      create_procedure (*db,
                        "person_age_decrement (IN min_age INTEGER,\n"
                                              "IN max_age INTEGER,\n"
                                              "OUT age SMALLINT,\n"
                                              "OUT first TEXT,\n"
                                              "OUT last TEXT)",
                        "AS $$\n"
                        "  UPDATE pgsql_stored_proc_person\n"
                        "    SET age = age - 1\n"
                        "    WHERE age >= min_age AND age <= max_age\n"
                        "    RETURNING age, first, last;\n"
                        "$$ LANGUAGE SQL;");

      typedef odb::query<person_age_decrement> query;
      typedef odb::result<person> result;

      // Return the first row of the multiple ones generated by the UPDATE
      // statement in the function implementation.
      //
      {
        transaction t (db->begin ());

        person_age_decrement fr (
          db->query_value<person_age_decrement> (
            query::_val (32) + "," + query::_val (33) + ", NULL, NULL, NULL"));

        cout << fr.first << " " << fr.last << " " << fr.age << endl
             << endl;

        result r (db->query<person> ("ORDER BY id"));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first () << " " << i->last () << " " << i->age () << endl;

        cout << endl;

        t.commit ();
      }

      // If the implementation statement(s) returns no rows, then the
      // procedure call ends up with the `procedure returned null record`
      // error.
      //
      try
      {
        transaction t (db->begin ());

        person_age_decrement fr (
          db->query_value<person_age_decrement> (
            query::_val (30) + "," + query::_val (30) + ", NULL, NULL, NULL"));

        assert (false);
      }
      catch (const odb::exception&) {}
    }

    // Assign values to the output parameters directly.
    //
    {
      create_procedure (*db,
                        "person_age_decrement (IN person_id BIGINT,\n"
                                              "OUT person_age SMALLINT,\n"
                                              "OUT person_first TEXT,\n"
                                              "OUT person_last TEXT)",
                        "AS $$\n"
                        "DECLARE\n"
                        "  a SMALLINT;\n"
                        "  f TEXT;\n"
                        "  l TEXT;\n"
                        "BEGIN\n"
                        "  IF person_id > 0 THEN\n"
                        "    UPDATE pgsql_stored_proc_person\n"
                        "      SET age = age - 1\n"
                        "      WHERE id=person_id\n"
                        "      RETURNING age, first, last INTO a, f, l;\n"
                        "    person_age := a;\n"
                        "    person_first := f;\n"
                        "    person_last := l;\n"
                        "  ELSE\n"
                        "    person_age=1000;\n"
                        "  END IF;\n"
                        "END;\n"
                        "$$ LANGUAGE PLPGSQL;");

      typedef odb::query<person_age_decrement> query;
      typedef odb::result<person> result;

      transaction t (db->begin ());

      // Assign the columns of the updated row to all the output parameters.
      //
      {
        person_age_decrement fr (
          db->query_value<person_age_decrement> (
            query::_val (1) + ", NULL, NULL, NULL"));

        cout << fr.first << " " << fr.last << " " << fr.age << endl
             << endl;

        result r (db->query<person> ("ORDER BY id"));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first () << " " << i->last () << " " << i->age () << endl;

        cout << endl;
      }

      // Assign value to a single output parameter and leave the remaining
      // ones unassigned, which ends up with the NULL values for them.
      //
      {
        person_age_decrement fr (
          db->query_value<person_age_decrement> (
            query::_val (0) + ", NULL, NULL, NULL"));

        cout << fr.first << " " << fr.last << " " << fr.age << endl
             << endl;

        result r (db->query<person> ("ORDER BY id"));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first () << " " << i->last () << " " << i->age () << endl;

        cout << endl;
      }

      t.commit ();
    }

    // No output parameters. Returns no rows.
    //
    {
      create_procedure (*db,
                        "person_age_decrement_void (IN person_id BIGINT)",
                        "AS $$\n"
                        "  UPDATE pgsql_stored_proc_person\n"
                        "    SET age = age - 1\n"
                        "    WHERE id=person_id;\n"
                        "$$ LANGUAGE SQL;");

      typedef odb::query<person_age_decrement_void> query;
      typedef odb::result<person> result;

      transaction t (db->begin ());

      {
        unique_ptr<person_age_decrement_void> fr (
          db->query_one<person_age_decrement_void> (query (query::_val (1))));

        assert (fr == nullptr);

        result r (db->query<person> ("ORDER BY id"));

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
          cout << i->first () << " " << i->last () << " " << i->age () << endl;

        cout << endl;
      }

      t.commit ();
    }

    // Perform internal transaction.
    //
    // Note that we can only call such a procedure using execute() since
    // query() cannot be called out of transaction. That also means that the
    // procedure cannot have parameters (neither input nor output).
    //
    {
      create_procedure (*db,
                        "decrement_all_ages ()",
                        "AS $$\n"
                        "BEGIN\n"
                        "  UPDATE pgsql_stored_proc_person\n"
                        "    SET age = age - 1;\n"
                        "  COMMIT;\n"
                        "END;\n"
                        "$$ LANGUAGE PLPGSQL;");

      {
        connection_ptr conn (db->connection ());
        conn->execute ("CALL decrement_all_ages()");
      }

      typedef odb::result<person> result;

      transaction t (db->begin ());

      result r (db->query<person> ("ORDER BY id"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->first () << " " << i->last () << " " << i->age () << endl;

      cout << endl;

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
