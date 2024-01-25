// file      : common/inverse/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test bidirectional relationships with inverse sides.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

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

    // Test raw pointers.
    //
    {
      using namespace test1;

      obj1_ptr o1_1 (new obj1);
      obj1_ptr o1_2 (new obj1);
      obj2_ptr o2 (new obj2);
      obj3_ptr o3_1 (new obj3);
      obj3_ptr o3_2 (new obj3);
      obj4_ptr o4_1 (new obj4);
      obj4_ptr o4_2 (new obj4);
      obj5_ptr o5_1 (new obj5);
      obj5_ptr o5_2 (new obj5);
      obj5_ptr o5_3 (new obj5);
      obj5_ptr o5_4 (new obj5);

      o1_1->id = "obj1 1";
      o1_1->o2 = o2;
      o1_1->o3.insert (o3_1);
      o1_1->o3.insert (o3_2);
      o1_1->o4 = o4_1;
      o1_1->o5.insert (o5_1);
      o1_1->o5.insert (o5_2);

      o1_2->id = "obj1 2";
      o1_2->o2 = 0;
      o1_2->o3.clear ();
      o1_2->o4 = o4_2;
      o1_2->o5.insert (o5_3);
      o1_2->o5.insert (o5_4);

      o2->str = "obj2";
      o2->o1 = o1_1;

      o3_1->str = "obj3 1";
      o3_1->o1 = o1_1;

      o3_2->str = "obj3 2";
      o3_2->o1 = o1_1;

      o4_1->str = "obj4 1";
      o4_1->o1.insert (o1_1);

      o4_2->str = "obj4 2";
      o4_2->o1.insert (o1_2);

      o5_1->str = "obj5 1";
      o5_1->o1.insert (o1_1);

      o5_2->str = "obj5 2";
      o5_2->o1.insert (o1_1);

      o5_3->str = "obj5 3";
      o5_3->o1.insert (o1_2);

      o5_4->str = "obj5 4";
      o5_4->o1.insert (o1_2);

      // persist
      //
      {
        transaction t (db->begin ());

        // objN come before obj1 to get object id assigned.
        //
        db->persist (o5_1);
        db->persist (o5_2);
        db->persist (o5_3);
        db->persist (o5_4);
        db->persist (o4_1);
        db->persist (o4_2);
        db->persist (o3_1);
        db->persist (o3_2);
        db->persist (o2);
        db->persist (o1_1);
        db->persist (o1_2);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        obj2_ptr x2 (db->load<obj2> (o2->id));
        obj3_ptr x3_1 (db->load<obj3> (o3_1->id));
        obj3_ptr x3_2 (db->load<obj3> (o3_2->id));
        obj4_ptr x4_1 (db->load<obj4> (o4_1->id));
        obj4_ptr x4_2 (db->load<obj4> (o4_2->id));
        obj5_ptr x5_1 (db->load<obj5> (o5_1->id));
        obj5_ptr x5_2 (db->load<obj5> (o5_2->id));
        obj5_ptr x5_3 (db->load<obj5> (o5_3->id));
        obj5_ptr x5_4 (db->load<obj5> (o5_4->id));
        t.commit ();

        assert (x2->str == o2->str);
        assert (x2->o1->id == o1_1->id);
        assert (x2->o1->o2 == x2);

        assert (x3_1->str == o3_1->str);
        assert (x3_2->str == o3_2->str);
        assert (x3_1->o1 == x3_2->o1);
        assert (x3_1->o1->id == o1_1->id);
        assert (x3_1->o1->o3.find (x3_1) != x3_1->o1->o3.end ());
        assert (x3_1->o1->o3.find (x3_2) != x3_1->o1->o3.end ());

        assert (x4_1->str == o4_1->str);
        assert (x4_2->str == o4_2->str);
        assert ((*x4_1->o1.begin ())->id == o1_1->id);
        assert ((*x4_2->o1.begin ())->id == o1_2->id);
        assert ((*x4_1->o1.begin ())->o4 == x4_1);
        assert ((*x4_2->o1.begin ())->o4 == x4_2);

        assert (x5_1->str == o5_1->str);
        assert (x5_2->str == o5_2->str);
        assert ((*x5_1->o1.begin ())->id == o1_1->id);
        assert ((*x5_2->o1.begin ())->id == o1_1->id);
        assert ((*x5_3->o1.begin ())->id == o1_2->id);
        assert ((*x5_4->o1.begin ())->id == o1_2->id);
        assert ((*x5_1->o1.begin ())->o5.find (x5_1) !=
                (*x5_1->o1.begin ())->o5.end ());
        assert ((*x5_2->o1.begin ())->o5.find (x5_2) !=
                (*x5_2->o1.begin ())->o5.end ());
        assert ((*x5_3->o1.begin ())->o5.find (x5_3) !=
                (*x5_3->o1.begin ())->o5.end ());
        assert ((*x5_4->o1.begin ())->o5.find (x5_4) !=
                (*x5_4->o1.begin ())->o5.end ());

        delete *x4_1->o1.begin ();
        delete *x4_2->o1.begin ();
      }

      // query
      //
      {
        // one(i)-to-one
        //
        typedef odb::query<obj2> query;
        typedef odb::result<obj2> result;

        session s;
        transaction t (db->begin ());

        result r (db->query<obj2> (query::o1->id == "obj1 1"));
        assert (!r.empty ());
        assert (r.begin ()->id == o2->id);
        assert (r.begin ()->o1->id == o1_1->id);
        assert (size (r) == 1);

        t.commit ();
      }

      {
        // one(i)-to-many
        //
        typedef odb::query<obj3> query;
        typedef odb::result<obj3> result;

        session s;
        transaction t (db->begin ());

        result r (db->query<obj3> (query::o1->id == "obj1 1"));
        size_t n (0);

        for (result::iterator i (r.begin ()); i != r.end (); ++i)
        {
          assert (i->id == o3_1->id || i->id == o3_2->id);
          assert (i->o1->id == o1_1->id);
          n++;
        }

        assert (n == 2);

        t.commit ();
      }

      delete o1_1;
      delete o1_2;
    }

    // Test shared_ptr/weak_ptr.
    //
    {
      using namespace test2;

      obj1_ptr o1_1 (new obj1);
      obj1_ptr o1_2 (new obj1);
      obj2_ptr o2 (new obj2);
      obj3_ptr o3_1 (new obj3);
      obj3_ptr o3_2 (new obj3);
      obj4_ptr o4 (new obj4);
      obj5_ptr o5_1 (new obj5);
      obj5_ptr o5_2 (new obj5);

      o1_1->id = "obj1 1";
      o1_1->o2 = o2;
      o1_1->o3.push_back (o3_1);
      o1_1->o3.push_back (o3_2);
      o1_1->o4 = o4;
      o1_1->o5.push_back (o5_1);
      o1_1->o5.push_back (o5_2);

      o1_2->id = "obj1 2";
      o1_2->o2 = obj2_ptr ();
      o1_2->o3.clear ();
      o1_2->o4 = o4;
      o1_2->o5.push_back (o5_1);

      o2->str = "obj2";
      o2->o1 = o1_1;

      o3_1->str = "obj3 1";
      o3_1->o1 = o1_1;

      o3_2->str = "obj3 3";
      o3_2->o1 = o1_1;

      o4->str = "obj4";
      o4->o1.push_back (o1_1);
      o4->o1.push_back (o1_2);

      o5_1->str = "obj5 1";
      o5_1->o1.push_back (o1_1);
      o5_1->o1.push_back (o1_2);

      o5_2->str = "obj5 2";
      o5_2->o1.push_back (o1_1);

      // persist
      //
      {
        transaction t (db->begin ());

        // objN come before obj1 to get object id assigned.
        //
        db->persist (o5_1);
        db->persist (o5_2);
        db->persist (o4);
        db->persist (o3_1);
        db->persist (o3_2);
        db->persist (o2);
        db->persist (o1_1);
        db->persist (o1_2);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        obj2_ptr x2 (db->load<obj2> (o2->id));
        obj3_ptr x3_1 (db->load<obj3> (o3_1->id));
        obj3_ptr x3_2 (db->load<obj3> (o3_2->id));
        obj4_ptr x4 (db->load<obj4> (o4->id));
        obj5_ptr x5_1 (db->load<obj5> (o5_1->id));
        obj5_ptr x5_2 (db->load<obj5> (o5_2->id));
        t.commit ();

        assert (x2->str == o2->str);
        assert (x2->o1.lock ()->id == o1_1->id);
        assert (x2->o1.lock ()->o2 == x2);

        assert (x3_1->str == o3_1->str);
        assert (x3_2->str == o3_2->str);
        assert (x3_1->o1.lock () == x3_2->o1.lock ());
        assert (x3_1->o1.lock ()->id == o1_1->id);
        assert (x3_1->o1.lock ()->o3[0] == x3_1);
        assert (x3_1->o1.lock ()->o3[1] == x3_2);

        {
          assert (x4->str == o4->str);

          obj1_ptr t1 (x4->o1[0].lock ()), t2 (x4->o1[1].lock ());

          assert (t1->id == o1_1->id || t2->id == o1_1->id);
          assert (t1->id == o1_2->id || t2->id == o1_2->id);
        }

        {
          assert (x5_1->str == o5_1->str);
          assert (x5_2->str == o5_2->str);

          obj1_ptr t1 (x5_1->o1[0].lock ()), t2 (x5_1->o1[1].lock ()),
            t3 (x5_2->o1[0].lock ());

          assert (t1->id == o1_1->id || t2->id == o1_1->id);
          assert (t1->id == o1_2->id || t2->id == o1_2->id);
          assert (t3->id == o1_1->id);
        }
      }
    }

    // Test inverse based on points_to.
    //
    {
      using namespace test3;

      {
        obj1 o1 (1, 2);
        o1.o2 = new obj2;

        {
          transaction t (db->begin ());

          o1.o2->o1 = db->persist (o1);
          db->persist (o1.o2);

          t.commit ();
        }

        {
          transaction t (db->begin ());

          unique_ptr<obj1> p (db->load<obj1> (o1.id));
          assert (p->o2->id == o1.o2->id);

          t.commit ();
        }

        {
          typedef odb::query<obj1> query;

          transaction t (db->begin ());

          unique_ptr<obj1> p (db->query_one<obj1> (query::o2->o1.i == o1.id.i &&
                                                 query::o2->o1.j == o1.id.j));
          assert (p->o2->id == o1.o2->id);

          t.commit ();
        }
      }

      {
        obj3 o3;
        o3.o4.push_back (new obj4);
        o3.o4.push_back (new obj4);

        {
          transaction t (db->begin ());

          o3.o4[0]->o3 = o3.o4[1]->o3 = db->persist (o3);
          db->persist (o3.o4[0]);
          db->persist (o3.o4[1]);

          t.commit ();
        }

        {
          transaction t (db->begin ());

          unique_ptr<obj3> p (db->load<obj3> (o3.id));
          assert (p->o4[0]->id == o3.o4[0]->id);
          assert (p->o4[1]->id == o3.o4[1]->id);

          t.commit ();
        }

        {
          typedef odb::query<obj3> query;

          transaction t (db->begin ());

          unique_ptr<obj3> p (db->query_one<obj3> (query::id == o3.id));
          assert (p->o4[0]->id == o3.o4[0]->id);
          assert (p->o4[1]->id == o3.o4[1]->id);

          t.commit ();
        }
      }
    }

    // Test inverse with nested data members.
    //
    {
      using namespace test4;

      {
        obj1 o1;
        o1.o2 = new obj2;

        {
          transaction t (db->begin ());

          o1.o2->id.i = db->persist (o1);
          o1.o2->id.j = 123;
          db->persist (o1.o2);

          t.commit ();
        }

        {
          transaction t (db->begin ());

          unique_ptr<obj1> p (db->load<obj1> (o1.id));
          assert (p->o2->id.i == o1.o2->id.i && p->o2->id.j == o1.o2->id.j);

          t.commit ();
        }

        {
          typedef odb::query<obj1> query;

          transaction t (db->begin ());

          unique_ptr<obj1> p (db->query_one<obj1> (
                              query::o2->id.i == o1.o2->id.i &&
                              query::o2->id.j == o1.o2->id.j));
          assert (p->o2->id.i == o1.o2->id.i && p->o2->id.j == o1.o2->id.j);

          t.commit ();
        }
      }

      {
        obj3 o3;
        o3.o4.push_back (new obj4);
        o3.o4.push_back (new obj4);

        {
          transaction t (db->begin ());

          o3.o4[0]->id.i = o3.o4[1]->id.i = db->persist (o3);
          o3.o4[0]->id.j = 123;
          o3.o4[1]->id.j = 234;
          db->persist (o3.o4[0]);
          db->persist (o3.o4[1]);

          t.commit ();
        }

        {
          transaction t (db->begin ());

          unique_ptr<obj3> p (db->load<obj3> (o3.id));
          assert (p->o4[0]->id.i == o3.o4[0]->id.i &&
                  p->o4[0]->id.j == o3.o4[0]->id.j);

          assert (p->o4[1]->id.i == o3.o4[1]->id.i &&
                  p->o4[1]->id.j == o3.o4[1]->id.j);

          t.commit ();
        }

        {
          typedef odb::query<obj3> query;

          transaction t (db->begin ());

          unique_ptr<obj3> p (db->query_one<obj3> (query::id == o3.id));

          assert (p->o4[0]->id.i == o3.o4[0]->id.i &&
                  p->o4[0]->id.j == o3.o4[0]->id.j);

          assert (p->o4[1]->id.i == o3.o4[1]->id.i &&
                  p->o4[1]->id.j == o3.o4[1]->id.j);

          t.commit ();
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
