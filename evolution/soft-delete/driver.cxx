// file      : evolution/soft-delete/driver.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test soft-delete functionality.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <common/common.hxx>

#include "test2.hxx"
#include "test3.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv, false));
    bool embedded (schema_catalog::exists (*db));

    // 1 - base version
    // 2 - migration
    // 3 - current version
    //
    unsigned short pass (*argv[argc - 1] - '0');

    switch (pass)
    {
    case 1:
      {
        using namespace v2;

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::drop_schema (*db);
          schema_catalog::drop_schema (*db, "2");
          schema_catalog::create_schema (*db, "", false);
          schema_catalog::migrate_schema (*db, 2);
          t.commit ();
        }

        // Test soft-deleted objects.
        //
        {
          using namespace test1;

          object o (1);
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // SQLite doesn't support dropping of columns.
        //
#ifndef DATABASE_SQLITE

        // Test basic soft-deleted member logic.
        //
        {
          using namespace test2;

          object o (1);
          o.str = "abc";
          o.num = 123;
          o.vec.push_back (123);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test container with soft-deleted value member.
        //
        {
          using namespace test5;

          object o (1);
          o.vec.push_back (value ("abc", 123));

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

#endif // DATABASE_SQLITE

        // Test soft-deleted container member in a non-versioned object.
        //
        {
          using namespace test21;

          object o (1);
          o.num = 123;
          o.vec.push_back (123);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        break;
      }
    case 2:
      {
        using namespace v3;

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_pre (*db, 3);
          t.commit ();
        }

        // Test soft-deleted objects.
        //
        {
          using namespace test1;

          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->num == 123);
            t.commit ();
          }
        }

        // SQLite doesn't support dropping of columns.
        //
#ifndef DATABASE_SQLITE

        // Test basic soft-deleted member logic.
        //
        {
          using namespace test2;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->str == "abc" && p->num == 123 && p->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end () &&
                    i->str == "abc" && i->num == 123 && i->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "bcd" && p->num == 234 && p->vec[0] == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "bcde" && p->num == 235 && p->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test container with soft-deleted value member.
        //
        {
          using namespace test5;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->vec[0].str == "abc" && p->vec[0].num == 123);
            t.commit ();
          }

          object o (2);
          o.vec.push_back (value ("bcd", 234));

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "bcd" && p->vec[0].num == 234);
            t.commit ();
          }

          o.vec.modify (0).str += 'e';
          o.vec.modify (0).num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "bcde" && p->vec[0].num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

#endif // DATABASE_SQLITE

        // Test soft-deleted container member in a non-versioned object.
        //
        {
          using namespace test21;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->num == 123 && p->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->num == 123 && i->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 234 && p->vec[0] == 234);
            t.commit ();
          }

          o.num++;
          o.vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 235 && p->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_post (*db, 3);
          t.commit ();
        }
        break;
      }
    case 3:
      {
        using namespace v3;

        // Test soft-deleted objects.
        //
        {
          using namespace test1;

          try
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1)); // No such table.
            assert (false);
          }
          catch (const odb::exception&) {}
        }

        // SQLite doesn't support dropping of columns.
        //
#ifndef DATABASE_SQLITE

        // Test basic soft-deleted member logic.
        //
        {
          using namespace test2;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->str == "" && p->num == 123 && p->vec.empty ());
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () &&
                    i->str == "" && i->num == 123 && i->vec.empty ());

            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}

            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "" && p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "" && p->num == 235 && p->vec.empty ());
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test empty statement handling (INSERT, UPDATE).
        //
        {
          using namespace test3;

          // Now none of the database operations should include the
          // deleted member.
          //
          object o;
          o.str = "bcd";

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "");
            t.commit ();
          }

          o.str += 'e';

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "");
            t.commit ();
          }
        }

        // Test empty statement handling (SELECT in polymorphic loader).
        //
        {
          using namespace test4;

          // Now none of the database operations should include the
          // deleted member.
          //
          object o (1);
          o.str = "abc";

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<base> p (db->load<base> (1));
            assert (static_cast<object&> (*p).str == "");
            t.commit ();
          }
        }

        // Test container with soft-deleted value member.
        //
        {
          using namespace test5;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->vec[0].str == "" && p->vec[0].num == 123);
            t.commit ();
          }

          object o (2);
          o.vec.push_back (value ("bcd", 234));

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "" && p->vec[0].num == 234);
            t.commit ();
          }

          o.vec.modify (0).str += 'e';
          o.vec.modify (0).num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "" && p->vec[0].num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

#endif // DATABASE_SQLITE

        // Test soft-deleted container member in a non-versioned object.
        //
        {
          using namespace test21;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->num == 123 && p->vec.empty ());
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->num == 123 && i->vec.empty ());
            t.commit ();
          }

          object o (2);
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.num++;
          o.vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 235 && p->vec.empty ());
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        break;
      }
    default:
      {
        cerr << "unknown pass number '" << argv[argc - 1] << "'" << endl;
        return 1;
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
