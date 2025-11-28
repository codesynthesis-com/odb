// file      : common/relationship/direct-load/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test direct load of object pointers (currently only in containers).
//
// Note: soft-add/delete-related tests are in ../../../evolution/.

#include <memory>   // std::unique_ptr
#include <cstddef>
#include <iostream>

#include <odb/tracer.hxx>
#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

struct select_counter: odb::tracer
{
  virtual void
  execute (connection&, const char* s)
  {
    if (string (s).compare (0, 6, "SELECT") == 0)
      count++;
  }

  std::size_t count = 0;
};

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    // Test unidirectional, non-inverse, pointer as container element.
    //
    {
      using namespace test1;

      {
        employer e (1);
        e.employees.push_back (make_shared<person> (1, "A"));
        e.employees.push_back (make_shared<person> (2, "B"));

        transaction t (db->begin ());
        db->persist (e.employees[0]);
        db->persist (e.employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 2 &&
                p->employees[0] != nullptr &&
                p->employees[0]->id == 1  &&
                p->employees[0]->name == "A" &&
                p->employees[1] != nullptr &&
                p->employees[1]->id == 2 &&
                p->employees[1]->name == "B" &&
                sc.count == 2);
        t.commit ();
      }

      // Test NULL pointer.
      //
      {
        employer e (2);
        e.employees.push_back (nullptr);

        transaction t (db->begin ());
        db->persist (e);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        shared_ptr<employer> p (db->load<employer> (2));
        assert (p->employees.size () == 1 &&
                p->employees[0] == nullptr);
        t.commit ();
      }
    }

    // Test many-to-one, inverse.
    //
    {
      using namespace test2;

      {
        shared_ptr<employer> e (make_shared<employer> (1));
        e->employees.push_back (make_shared<person> (1, "A", e));
        e->employees.push_back (make_shared<person> (2, "B", e));

        transaction t (db->begin ());
        db->persist (e->employees[0]);
        db->persist (e->employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 2 &&
                p->employees[0] != nullptr &&
                p->employees[0]->id == 1  &&
                p->employees[0]->name == "A" &&
                p->employees[0]->employer.lock () == p &&
                p->employees[1] != nullptr &&
                p->employees[1]->id == 2 &&
                p->employees[1]->name == "B" &&
                p->employees[1]->employer.lock () == p &&
                sc.count == 2);
        t.commit ();
      }

      {
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<person> p (db->load<person> (1));
        shared_ptr<employer> e (p->employer.lock ());
        assert (e != nullptr &&
                e->id == 1 &&
                e->employees.size () == 2 &&
                e->employees[0] == p &&
                sc.count == 3);
        t.commit ();
      }
    }

    // Test many-to-many, non-inverse, weak_ptr, both direct load.
    //
    {
      using namespace test3;

      {
        shared_ptr<employer> e1 (make_shared<employer> (1));
        shared_ptr<employer> e2 (make_shared<employer> (2));
        shared_ptr<person> p1 (make_shared<person> (1, "A"));
        shared_ptr<person> p2 (make_shared<person> (2, "B"));

        e1->employees.push_back (p1);
        e1->employees.push_back (p2);
        e1->data.push_back ("e1");

        e2->employees.push_back (p2);
        e2->data.push_back ("e2");

        p1->employers.push_back (e1);
        p1->data.push_back ("p1");

        p2->employers.push_back (e1);
        p2->employers.push_back (e2);
        p2->data.push_back ("p2");

        transaction t (db->begin ());
        db->persist (p1);
        db->persist (p2);
        db->persist (e1);
        db->persist (e2);
        t.commit ();
      }

      {
        // We expect the following SELECT statements:
        //
        // e1
        //   e1.employees
        //     p1.employers
        //     p1.data
        //     p2.employers
        //     p2.data
        //       e2.employees
        //       e2.data
        //   e1.data
        //
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> e1 (db->load<employer> (1)), e2;
        assert (e1->employees.size () == 2 &&

                e1->employees[0] != nullptr &&
                e1->employees[0]->id == 1  &&
                e1->employees[0]->name == "A" &&
                e1->employees[0]->employers.size () == 1 &&
                e1->employees[0]->employers[0].lock () == e1 &&

                e1->employees[1] != nullptr &&
                e1->employees[1]->id == 2  &&
                e1->employees[1]->name == "B" &&
                e1->employees[1]->employers.size () == 2 &&
                e1->employees[1]->employers[0].lock () == e1 &&
                (e2 = e1->employees[1]->employers[1].lock ()) != nullptr &&

                e2->id == 2 &&
                e2->data.size () == 1 && // Delayed.
                e2->data[0] == "e2" &&

                e1->data.size () == 1 &&
                e1->data[0] == "e1" &&

                sc.count == 9);
        t.commit ();
      }

      {
        // We expect the following SELECT statements:
        //
        // p2
        //   p2.employers
        //     e1.employees
        //       p1.employers
        //       p1.data
        //     e1.data
        //     e2.employees
        //     e2.data
        //   p2.data
        //
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<person> p2 (db->load<person> (2)), p1;
        shared_ptr<employer> e1, e2;
        assert (p2->employers.size () == 2 &&

                (e1 = p2->employers[0].lock ()) != nullptr &&
                e1->id == 1 &&
                e1->employees.size () == 2 &&
                (p1 = e1->employees[0]) != nullptr &&
                e1->employees[1] == p2 &&

                p1->id == 1 &&
                p1->data.size () == 1 && // Delayed.
                p1->data[0] == "p1" &&

                (e2 = p2->employers[1].lock ()) != nullptr &&
                e2->id == 2 &&
                e2->employees.size () == 1 &&
                e2->employees[0] == p2 &&

                p2->data.size () == 1 &&
                p2->data[0] == "p2" &&

                sc.count == 9);
        t.commit ();
      }
    }

    // Test composite pointed-to object id.
    //
    {
      using namespace test4;

      {
        employer e (1);
        e.employees.push_back (make_shared<person> (1, "x", "A"));
        e.employees.push_back (make_shared<person> (2, "y", "B"));

        transaction t (db->begin ());
        db->persist (e.employees[0]);
        db->persist (e.employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 2 &&
                p->employees[0] != nullptr &&
                p->employees[0]->id.i == 1  &&
                p->employees[0]->id.s == "x"  &&
                p->employees[0]->name == "A" &&
                p->employees[1] != nullptr &&
                p->employees[1]->id.i == 2  &&
                p->employees[1]->id.s == "y"  &&
                p->employees[1]->name == "B" &&
                sc.count == 2);
        t.commit ();
      }
    }

    // Test pointers in composite.
    //
    {
      using namespace test5;

      {
        shared_ptr<employer> e1 (make_shared<employer> (1));
        shared_ptr<team> t1 (make_shared<team> (1, "X", e1));
        shared_ptr<team> t2 (make_shared<team> (2, "Y", e1));
        shared_ptr<person> p1 (make_shared<person> (1, "A", t1));
        shared_ptr<person> p2 (make_shared<person> (2, "B", t1));
        shared_ptr<person> p3 (make_shared<person> (3, "C", t2));

        e1->employees.push_back (employee {"CEO", p1, t1});
        e1->employees.push_back (employee {"CTO", p2, t1});
        e1->employees.push_back (employee {"CFO", p3, t2});

        transaction t (db->begin ());
        db->persist (p1);
        db->persist (p2);
        db->persist (p3);
        db->persist (t1);
        db->persist (t2);
        db->persist (e1);
        t.commit ();
      }

      {
        // We expect the following SELECT statements:
        //
        // e1
        //   e1.employees
        //
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> e1 (db->load<employer> (1));
        shared_ptr<person> p1, p2, p3;
        shared_ptr<team> t1, t2;
        assert (e1->employees.size () == 3 &&

                e1->employees[0].title == "CEO" &&
                (p1 = e1->employees[0].person) != nullptr &&
                p1->id == 1 &&
                p1->name == "A" &&
                p1->team == (t1 = e1->employees[0].team.lock ()) &&

                e1->employees[1].title == "CTO" &&
                (p2 = e1->employees[1].person) != nullptr &&
                p2->id == 2 &&
                p2->name == "B" &&
                e1->employees[1].team.lock () == t1 &&
                p2->team == t1 &&

                e1->employees[2].title == "CFO" &&
                (p3 = e1->employees[2].person) != nullptr &&
                p3->id == 3 &&
                p3->name == "C" &&
                p3->team == (t2 = e1->employees[2].team.lock ()) &&

                t1->name == "X" &&
                t1->employer == e1 &&

                t2->name == "Y" &&
                t2->employer == e1 &&

                sc.count == 2);
        t.commit ();
      }

      {
        // We expect the following SELECT statements:
        //
        // p1
        //   t1
        //     e1
        //       e1.employees
        //
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<person> p1 (db->load<person> (1)), p2, p3;
        shared_ptr<employer> e1;
        shared_ptr<team> t1, t2;
        assert ((t1 = p1->team) != nullptr &&

                (e1 = t1->employer) != nullptr &&
                e1->id == 1 &&
                e1->employees.size () == 3 &&

                e1->employees[0].title == "CEO" &&
                e1->employees[0].person == p1 &&
                e1->employees[0].team.lock () == t1 &&

                e1->employees[1].title == "CTO" &&
                (p2 = e1->employees[1].person) != nullptr &&
                p2->id == 2 &&
                p2->name == "B" &&
                e1->employees[1].team.lock () == t1 &&
                p2->team == t1 &&

                e1->employees[2].title == "CFO" &&
                (p3 = e1->employees[2].person) != nullptr &&
                p3->id == 3 &&
                p3->name == "C" &&
                p3->team == (t2 = e1->employees[2].team.lock ()) &&

                t1->name == "X" &&
                t1->employer == e1 &&

                t2->name == "Y" &&
                t2->employer == e1 &&

                sc.count == 4);
        t.commit ();
      }
    }

    // Test smart container.
    //
    {
      using namespace test6;

      employer e (1);
      e.employees.push_back (make_shared<person> (1, "A"));
      e.employees.push_back (make_shared<person> (2, "B"));

      {
        transaction t (db->begin ());
        db->persist (e.employees[0]);
        db->persist (e.employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 2 &&
                p->employees[0] != nullptr &&
                p->employees[0]->id == 1 &&
                p->employees[0]->name == "A" &&
                p->employees[1] != nullptr &&
                p->employees[1]->id == 2 &&
                p->employees[1]->name == "B" &&
                sc.count == 2);
        t.commit ();
      }

      // Insert.
      //
      {
        e.employees.push_back (make_shared<person> (3, "C"));

        transaction t (db->begin ());
        db->persist (e.employees[2]);
        db->update (e);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 3 &&
                p->employees[0]->id == 1 &&
                p->employees[1]->id == 2 &&
                p->employees[2] != nullptr &&
                p->employees[2]->id == 3 &&
                p->employees[2]->name == "C");
        t.commit ();
      }

      // Update.
      //
      {
        e.employees.modify (1) = make_shared<person> (4, "D");

        transaction t (db->begin ());
        db->persist (e.employees[1]);
        db->update (e);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 3 &&
                p->employees[0]->id == 1 &&
                p->employees[1]->id == 4 &&
                p->employees[1]->name == "D" &&
                p->employees[2]->id == 3);
        t.commit ();
      }

      // Delete.
      //
      {
        e.employees.pop_back ();

        transaction t (db->begin ());
        db->update (e);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->employees.size () == 2 &&
                p->employees[0]->id == 1 &&
                p->employees[1]->id == 4);
        t.commit ();
      }
    }

    // Test container in composite value.
    //
    {
      using namespace test7;

      {
        employer e (1);
        e.hr.employees.push_back (make_shared<person> (1, "A"));
        e.hr.employees.push_back (make_shared<person> (2, "B"));

        transaction t (db->begin ());
        db->persist (e.hr.employees[0]);
        db->persist (e.hr.employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> p (db->load<employer> (1));
        assert (p->hr.employees.size () == 2 &&
                p->hr.employees[0] != nullptr &&
                p->hr.employees[0]->id == 1  &&
                p->hr.employees[0]->name == "A" &&
                p->hr.employees[1] != nullptr &&
                p->hr.employees[1]->id == 2 &&
                p->hr.employees[1]->name == "B" &&
                sc.count == 2);
        t.commit ();
      }
    }

    // Test sections in pointed-to object.
    //
    {
      using namespace test8;

      {
        employer e (1);
        e.employees.push_back (make_shared<person> (1, "A", "X"));
        e.employees.push_back (make_shared<person> (2, "B", "Y"));

        transaction t (db->begin ());
        db->persist (e.employees[0]);
        db->persist (e.employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> e (db->load<employer> (1));
        shared_ptr<person> p1, p2;
        assert (e->employees.size () == 2 &&
                (p1 = e->employees[0]) != nullptr &&
                p1->id == 1  &&
                p1->fname == "A" &&
                !p1->sname_s.loaded () &&

                (p2 = e->employees[1]) != nullptr &&
                p2->id == 2 &&
                p2->fname == "B" &&
                !p2->sname_s.loaded () &&

                sc.count == 2);

        db->load (*p1, p1->sname_s);
        db->load (*p2, p2->sname_s);

        assert (p1->sname == "X" &&
                p2->sname == "Y");

        p1->fname = "C";
        p1->sname = "Z";

        db->update (p1);
        db->update (*p1, p1->fname_s);

        t.commit ();
      }

      {
        transaction t (db->begin ());
        shared_ptr<person> p1 (db->load<person> (1));
        db->load (*p1, p1->sname_s);
        assert (p1->fname == "C" &&
                p1->sname == "Z");
      }
    }

    // Test direct load of map key.
    //
    {
      using namespace test9;

      {
        employer e (1);
        shared_ptr<person> p1 (make_shared<person> (1, "A"));
        shared_ptr<person> p2 (make_shared<person> (2, "B"));
        e.employees.emplace (p1, string ("CEO"));
        e.employees.emplace (p2, string ("CTO"));

        transaction t (db->begin ());
        db->persist (p1);
        db->persist (p2);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> e (db->load<employer> (1));
        assert (e->employees.size () == 2 &&
                sc.count == 2);

        employees_map::iterator i1, i2;

        i1 = i2 = e->employees.begin ();
        (e->employees.begin ()->first->id == 1 ? i2 : i1)++;

        assert (i1->first->name == "A" &&
                i1->second == "CEO" &&

                i2->first->name == "B" &&
                i2->second == "CTO");
        t.commit ();
      }
    }

    // Test direct load of map key and value.
    //
    {
      using namespace test10;

      {
        employer e (1);
        shared_ptr<team> t1 (make_shared<team> (1, "X"));
        shared_ptr<person> p1 (make_shared<person> (1, "A"));
        shared_ptr<person> p2 (make_shared<person> (2, "B"));
        e.employees.emplace (p1, t1);
        e.employees.emplace (p2, t1);

        transaction t (db->begin ());
        db->persist (p1);
        db->persist (p2);
        db->persist (t1);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        session s;
        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> e (db->load<employer> (1));
        assert (e->employees.size () == 2 &&
                sc.count == 2);

        employees_map::iterator i1, i2;

        i1 = i2 = e->employees.begin ();
        (e->employees.begin ()->first->id == 1 ? i2 : i1)++;

        assert (i1->first->name == "A" &&
                i1->second->id == 1 &&
                i1->second->name == "X" &&

                i2->first->name == "B" &&
                i2->second->id == 1 &&
                i2->second->name == "X" &&

                i1->second == i2->second);
        t.commit ();
      }
    }

    // Test direct load container in section.
    //
    {
      using namespace test11;

      employer e (1);
      e.employees.push_back (make_shared<person> (1, "A"));
      e.employees.push_back (make_shared<person> (2, "B"));

      {
        transaction t (db->begin ());
        db->persist (e.employees[0]);
        db->persist (e.employees[1]);
        db->persist (e);
        t.commit ();
      }

      {
        select_counter sc;

        transaction t (db->begin ());
        t.tracer (sc);
        shared_ptr<employer> e (db->load<employer> (1));
        assert (!e->employees_s.loaded () &&
                e->employees.size () == 0 &&
                sc.count == 1);
        db->load (*e, e->employees_s);
        assert (e->employees.size () == 2 &&
                e->employees[0] != nullptr &&
                e->employees[0]->id == 1  &&
                e->employees[0]->name == "A" &&
                e->employees[1] != nullptr &&
                e->employees[1]->id == 2 &&
                e->employees[1]->name == "B" &&
                sc.count == 2);
        t.commit ();
      }

      e.employees.pop_back ();

      {
        transaction t (db->begin ());
        db->update (e);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        shared_ptr<employer> e (db->load<employer> (1));
        db->load (*e, e->employees_s);
        assert (e->employees.size () == 1 &&
                e->employees[0] != nullptr &&
                e->employees[0]->id == 1  &&
                e->employees[0]->name == "A");
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
