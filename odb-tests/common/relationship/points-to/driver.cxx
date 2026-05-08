// file      : common/relationship/points-to/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test establishment of object relationships using points_to pragma.
//

#include <memory>   // std::unique_ptr, std::shared_ptr, std::make_shared()
#include <vector>
#include <string>
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

#include <libcommon/config.hxx> // DATABASE_XXX
#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    // Test column constraints generation for container members.
    //
    {
      using namespace test1;

      // Simple id.
      //
      {
        using namespace simple_id;

        shared_ptr<obj> o1 (make_shared<obj> (1));
        o1->ids.push_back (o1->id);
        o1->objs.push_back (o1);
        o1->objs.push_back (nullptr);

        shared_ptr<obj> o2 (make_shared<obj> (2));
        o2->ids.push_back (o1->id);
        o2->ids.push_back (o2->id);
        o2->objs.push_back (o1);
        o2->objs.push_back (o2);

        {
          transaction t (db->begin ());
          db->persist (o1);
          db->persist (o2);
          t.commit ();
        }

        {
          session s; // Prevent infinite recursion due to reference cycles.

          transaction t (db->begin ());
          obj v (db->query_value<obj> (query<obj>::id == o1->id));
          shared_ptr<obj> p (db->load<obj> (o2->id));
          t.commit ();

          assert (v == *o1);
          assert (*p == *o2);

          // Get rid of the potential reference cycles prior to the objects
          // destruction.
          //
          v.objs.clear ();
          p->objs.clear ();
        }

        // Test that update fails if the points_to-declared or pointer member
        // refers to an object which is not persisted.
        //
        // Note that MySQL and SQL Server do not support deferrable constraint
        // checking (see odb/relational/{mysql,mssql}/schema.cxx for details),
        // so no exception would be thrown for them.
        //
#if !defined(DATABASE_MYSQL) && !defined(DATABASE_MSSQL)
        o1->ids.push_back (1000);

        try
        {
          transaction t (db->begin ());
          db->update (o1);
          t.commit ();

          assert (false);
        }
        catch (const odb::exception&) {}

        o2->objs.push_back (make_shared<obj> (1000));

        try
        {
          transaction t (db->begin ());
          db->update (o2);
          t.commit ();

          assert (false);
        }
        catch (const odb::exception&) {}
#endif

        // Get rid of the potential reference cycles prior to the objects
        // destruction.
        //
        o1->objs.clear ();
        o2->objs.clear ();
      }

      // Composite id.
      //
      {
        using namespace composite_id;

        shared_ptr<obj> o1 (make_shared<obj> (1, "aaa"));
        o1->ids.push_back (o1->id);
        o1->objs.push_back (o1);
        o1->objs.push_back (nullptr);

        shared_ptr<obj> o2 (make_shared<obj> (2, "bbb"));
        o2->ids.push_back (o1->id);
        o2->ids.push_back (o2->id);
        o2->objs.push_back (o1);
        o2->objs.push_back (o2);

        {
          transaction t (db->begin ());
          db->persist (o1);
          db->persist (o2);
          t.commit ();
        }

        {
          session s; // Prevent infinite recursion due to reference cycles.

          transaction t (db->begin ());

          using query = query<obj>;

          obj v (db->query_value<obj> (query::id.num == o1->id.num &&
                                       query::id.str == o1->id.str));

          shared_ptr<obj> p (db->load<obj> (o2->id));

          t.commit ();

          assert (v == *o1);
          assert (*p == *o2);

          // Get rid of the potential reference cycles prior to the objects
          // destruction.
          //
          v.objs.clear ();
          p->objs.clear ();
        }

        // Test that update fails if the referred to object is not persisted
        // (see above for details).
        //
#if !defined(DATABASE_MYSQL) && !defined(DATABASE_MSSQL)
        o1->ids.emplace_back (1000, "ccc");

        try
        {
          transaction t (db->begin ());
          db->update (o1);
          t.commit ();

          assert (false);
        }
        catch (const odb::exception&) {}

        o2->objs.push_back (make_shared<obj> (1000, "ccc"));

        try
        {
          transaction t (db->begin ());
          db->update (o2);
          t.commit ();

          assert (false);
        }
        catch (const odb::exception&) {}
#endif

        // Get rid of the potential reference cycles prior to the objects
        // destruction.
        //
        o1->objs.clear ();
        o2->objs.clear ();
      }
    }

    // Test association via points_to-declared members in object views.
    //
    {
      using namespace test2;

      // Simple id.
      //
      {
        using namespace simple_id;

        shared_ptr<obj1> o1 (make_shared<obj1> (1, 10));
        shared_ptr<obj2> o2 (make_shared<obj2> (2, 20, o1->id));
        shared_ptr<obj3> o3 (make_shared<obj3> (3, 30, o1));

        {
          transaction t (db->begin ());
          db->persist (o1);
          db->persist (o2);
          db->persist (o3);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          view1 v1 (db->query_value<view1> ());
          view2 v2 (db->query_value<view2> ());
          t.commit ();

          assert (v1.field1 == 10);
          assert (v1.field2 == 20);

          assert (v2.field1 == 10);
          assert (v2.field3 == 30);
        }
      }

      // Composite id.
      //
      {
        using namespace composite_id;

        shared_ptr<obj1> o1 (make_shared<obj1> (1, "aaa", 10));
        shared_ptr<obj2> o2 (make_shared<obj2> (2, 20, o1->id));
        shared_ptr<obj3> o3 (make_shared<obj3> (3, 30, o1));

        {
          transaction t (db->begin ());
          db->persist (o1);
          db->persist (o2);
          db->persist (o3);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          view1 v1 (db->query_value<view1> ());
          view2 v2 (db->query_value<view2> ());
          t.commit ();

          assert (v1.field1 == 10);
          assert (v1.field2 == 20);

          assert (v2.field1 == 10);
          assert (v2.field3 == 30);
        }
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
