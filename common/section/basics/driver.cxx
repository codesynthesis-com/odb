// file      : common/section/basics/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test object section basics.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

struct failed {};

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Test lazy-loaded, always updating section.
    //
    {
      using namespace test1;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // We can also update just the section.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n != o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // Test updating unloaded section.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;
      o.s.unload ();

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);
        t.commit ();
      }

      // Test reloading of loaded/unloaded sections.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;
      o.s.unload ();

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        db->reload (o);
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);
        db->load (o, o.s);
        t.commit ();
      }

      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        db->reload (o);
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      typedef odb::query<object> query;
      typedef odb::result<object> result;

      // Make sure we can access section members in queries.
      //
      {
        transaction t (db->begin ());

        result r (db->query<object> (query::ss == o.ss));
        result::iterator i (r.begin ());

        assert (i != r.end () && !i->s.loaded ());

        db->load (*i, i->s);
        assert (i->n == o.n &&
                i->sn == o.sn && i->ss == o.ss && i->sv == o.sv);

        assert (++i == r.end ());

        t.commit ();
      }

      // Make sure we can load/update sections without messing up the
      // loaded object's image.
      //
      {
        transaction t (db->begin ());

        result r (db->query<object> (query::ss == o.ss));
        result::iterator i (r.begin ());

        assert (i != r.end ());

        object o1;
        i.load (o1);
        db->load (o1, o1.s);
        assert (o1.n == o.n &&
                o1.sn == o.sn && o1.ss == o.ss && o1.sv == o.sv);

        o.sn++;
        o.ss += 'd';
        o.sv[0]++;
        db->update (o, o.s);

        object o2;
        i.load (o2);
        db->load (o2, o2.s);
        assert (o2.n == o1.n &&
                o2.sn == o.sn && o2.ss == o.ss && o2.sv == o.sv);

        assert (++i == r.end ());

        t.commit ();
      }
    }

    // Test lazy-loaded, change-updated section.
    //
    {
      using namespace test2;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update but don't mark as changed.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);
        t.commit ();
      }

      // Mark as changed.
      //
      o.s.change ();

      {
        transaction t (db->begin ());
        db->update (o);
        assert (!o.s.changed ());
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // We can also update just the section manually.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;
      o.s.change ();

      {
        transaction t (db->begin ());
        db->update (o, o.s);
        assert (!o.s.changed ());
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n != o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }
    }

    // Test lazy-loaded, manually-updated section.
    //
    {
      using namespace test3;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update the object only.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);
        t.commit ();
      }

      // Update both the object and section.
      //
      o.n++;

      {
        transaction t (db->begin ());
        db->update (o);
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // We can also update just the section.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n != o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // Test detection of unloaded section update.
      //
      try
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->update (*p, p->s);
        assert (false);
      }
      catch (const section_not_loaded&)
      {
      }
    }

    // Test eager-loaded, change-updated section.
    //
    {
      using namespace test4;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update but don't mark as changed.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);
        t.commit ();
      }

      // Mark as changed.
      //
      o.s.change ();

      {
        transaction t (db->begin ());
        db->update (o);
        assert (!o.s.changed ());
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }
    }

    // Test eager-loaded, manually-updated section.
    //
    {
      using namespace test5;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update the object only.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);
        t.commit ();
      }

      // Update both the object and section.
      //
      o.n++;

      {
        transaction t (db->begin ());
        db->update (o);
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // We can also update just the section.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        assert (p->n != o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }
    }

    // Test value-only and container-only section. Also multiple sections
    // in an object.
    //
    {
      using namespace test6;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s1.loaded ());
        assert (o.s2.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s1.loaded ());
        assert (!p->s2.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);

        db->load (*p, p->s1);
        assert (p->s1.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv != o.sv);

        db->load (*p, p->s2);
        assert (p->s2.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s1);
        db->load (*p, p->s2);
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }

      // We can also update just the section.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o, o.s1);
        db->update (o, o.s2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s1);
        db->load (*p, p->s2);
        assert (p->n != o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);
        t.commit ();
      }
    }

    // Test value-only and container-only section. Also multiple sections
    // in an object.
    //
    {
      using namespace test7;

      object o (123, "abc", true);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s1.loaded ());
        assert (o.s2.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s1.loaded ());
        assert (!p->s2.loaded ());
        assert (p->sn1 != o.sn1 && p->ss1 != o.ss1 &&
                p->sn2 != o.sn2 && p->ss2 != o.ss2 && p->sb2 != o.sb2);

        db->load (*p, p->s1);
        db->load (*p, p->s2);
        assert (p->s1.loaded ());
        assert (p->s2.loaded ());
        assert (p->sn1 == o.sn1 && p->ss1 == o.ss1 &&
                p->sn2 == o.sn2 && p->ss2 == o.ss2 && p->sb2 == o.sb2);


        t.commit ();
      }

      // Update.
      //
      o.sn1++;
      o.sn2++;
      o.ss1 += 'd';
      o.ss2 += 'd';
      o.sb2 = !o.sb2;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s1);
        db->load (*p, p->s2);
        assert (p->sn1 == o.sn1 && p->ss1 == o.ss1 &&
                p->sn2 == o.sn2 && p->ss2 == o.ss2 && p->sb2 == o.sb2);
        t.commit ();
      }

      // Manual update of just the section.
      //
      o.sn1++;
      o.sn2++;
      o.ss1 += 'd';
      o.ss2 += 'd';
      o.sb2 = !o.sb2;

      {
        transaction t (db->begin ());
        db->update (o, o.s2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s1);
        db->load (*p, p->s2);
        assert (p->sn1 != o.sn1 && p->ss1 != o.ss1 &&
                p->sn2 == o.sn2 && p->ss2 == o.ss2 && p->sb2 == o.sb2);
        t.commit ();
      }
    }

    // Test readonly and inverse section members.
    //
    {
      using namespace test8;

      object1 o1 (new object (123, "abc"));
      object& o (*o1.p);
      o.sp = &o1;

      {
        transaction t (db->begin ());
        db->persist (o);
        db->persist (o1);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        session s;

        transaction t (db->begin ());
        auto_ptr<object1> p1 (db->load<object1> (o1.id));
        object* p (p1->p);

        assert (!p->s.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sp == 0);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sp->id == o.sp->id);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        session s;

        transaction t (db->begin ());
        auto_ptr<object1> p1 (db->load<object1> (o1.id));
        object* p (p1->p);

        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss == o.ss && p->sp->id == o.sp->id);

        t.commit ();
      }
    }

    // Test object without any columns to load or update.
    //
    {
      using namespace test9;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (o.id == o.id &&
                p->sn != o.sn && p->ss != o.ss);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (o.id == o.id &&
                p->sn == o.sn && p->ss == o.ss);

        t.commit ();
      }

      // Update object.
      //
      o.sn++;
      o.ss += 'd';

      {
        transaction t (db->begin ());
        db->update (o); // No-op.
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (o.id == o.id &&
                p->sn != o.sn && p->ss != o.ss);
        t.commit ();
      }

      // Update section.
      //
      {
        transaction t (db->begin ());
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (o.id == o.id &&
                p->sn == o.sn && p->ss == o.ss);
        t.commit ();
      }
    }

    // Test section without any columns or containers to update.
    //
    {
      using namespace test10;

      object o (123);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n && p->sn != o.sn);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n && p->sn == o.sn);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;

      {
        transaction t (db->begin ());
        db->update (o);
        //db->update (o, o.s); // Error.
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n && p->sn != o.sn);
        t.commit ();
      }
    }

    // Test section with composite member.
    //
    {
      using namespace test11;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->sc.s != o.sc.s && p->sc.v != o.sc.v);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->sc.s == o.sc.s && p->sc.v == o.sc.v);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.sc.s += 'd';
      o.sc.v[0]++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n &&
                p->sn == o.sn && p->sc.s == o.sc.s && p->sc.v == o.sc.v);
        t.commit ();
      }
    }

    // Test change state restoration on transaction rollback.
    //
    {
      using namespace test12;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';
      o.s.change ();

      try
      {
        transaction t (db->begin ());
        db->update (o);
        assert (!o.s.changed ());
        throw failed ();
      }
      catch (const failed&)
      {
        assert (o.s.changed ());
      }

      // Retry. Also test the object destruction before transaction
      // termination case.
      //
      {
        transaction t (db->begin ());
        {
          object c (o);
          db->update (c);
          assert (!c.s.changed ());
        }
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n && p->sn == o.sn && p->ss == o.ss);
        t.commit ();
      }
    }

    // Test section accessor/modifier.
    //
    {
      using namespace test13;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s ().loaded ());
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s ().loaded ());
        assert (p->n == o.n && p->sn != o.sn && p->ss != o.ss);

        db->load (*p, p->rw_s ());

        assert (p->s ().loaded ());
        assert (p->n == o.n && p->sn == o.sn && p->ss == o.ss);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.ss += 'd';

      {
        transaction t (db->begin ());
        db->update (o);
        db->update (o, o.s ());
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->rw_s ());
        assert (p->n == o.n && p->sn == o.sn && p->ss == o.ss);
        t.commit ();
      }

      // Test detection of section copy.
      //
      try
      {
        transaction t (db->begin ());
        section c (o.s ());
        db->update (o, c);
        assert (false);
      }
      catch (const section_not_in_object&)
      {
      }
    }

    // Test LOB in section streaming, column re-ordering.
    //
    {
      using namespace test14;

      object o (1, 123, "\x01\x02\x03\x04\x05");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n && p->sn != o.sn && p->sb != o.sb);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n && p->sn == o.sn && p->sb == o.sb);

        t.commit ();
      }

      // Update.
      //
      o.n++;
      o.sn++;
      o.sb.push_back ('\x06');

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n == o.n && p->sn == o.sn && p->sb == o.sb);
        t.commit ();
      }

      // We can also update just the section.
      //
      o.n++;
      o.sn++;
      o.sb.push_back ('\x07');

      {
        transaction t (db->begin ());
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->n != o.n && p->sn == o.sn && p->sb == o.sb);
        t.commit ();
      }
    }

    // Test sections and optimistic concurrency.
    //
    {
      using namespace test15;

      object o (123, "abc");

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->n == o.n &&
                p->sn != o.sn && p->ss != o.ss && p->sv != o.sv);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->n == o.n &&
                p->sn == o.sn && p->ss == o.ss && p->sv == o.sv);

        t.commit ();
      }

      // Update object.
      //
      object o1 (o);
      o1.n++;
      o1.sn++;
      o1.ss += 'd';
      o1.sv[0]++;

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        db->update (o1);
        assert (o.v != o1.v);

        try
        {
          db->load (*p, p->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*p);
          assert (!p->s.loaded ());
          db->load (*p, p->s);

          assert (p->n == o1.n &&
                  p->sn == o1.sn && p->ss == o1.ss && p->sv == o1.sv);
        }

        db->reload (o);
        assert (o.v == o1.v);
        assert (o.n == o1.n &&
                o.sn == o1.sn && o.ss == o1.ss && o.sv == o1.sv);
        t.commit ();
      }

      // Update section.
      //
      o1.sn++;
      o1.ss += 'd';
      o1.sv[0]++;

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        db->update (o1, o1.s);
        assert (o.v != o1.v);

        // Double-check object version was updated.
        //
        {
          auto_ptr<object> p1 (db->load<object> (o.id));
          assert (o1.v == p1->v);
        }

        try
        {
          db->load (*p, p->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*p);
          assert (!p->s.loaded ());
          db->load (*p, p->s);

          assert (p->n == o1.n &&
                  p->sn == o1.sn && p->ss == o1.ss && p->sv == o1.sv);
        }

        db->reload (o);
        assert (o.v == o1.v);
        assert (o.n == o1.n &&
                o.sn == o1.sn && o.ss == o1.ss && o.sv == o1.sv);
        t.commit ();
      }

      // Update changed section.
      //
      o1.sn++;
      o1.ss += 'd';
      o1.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o1, o1.s);

        try
        {
          db->update (o, o.s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (o);
          db->update (o, o.s);
        }

        t.commit ();
      }
    }

    // Test container-only sections and optimistic concurrency.
    //
    {
      using namespace test16;

      object o (123);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->sv != o.sv);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->sv == o.sv);

        t.commit ();
      }

      // Update object.
      //
      object o1 (o);
      o1.sv[0]++;

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        db->update (o1);
        assert (o.v != o1.v);

        try
        {
          db->load (*p, p->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*p);
          assert (!p->s.loaded ());
          db->load (*p, p->s);

          assert (p->sv == o1.sv);
        }

        db->reload (o);
        assert (o.v == o1.v);
        assert (o.sv == o1.sv);
        t.commit ();
      }

      // Update section.
      //
      o1.sv[0]++;

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        db->update (o1, o1.s);
        assert (o.v != o1.v);

        // Double-check object version was updated.
        //
        {
          auto_ptr<object> p1 (db->load<object> (o.id));
          assert (o1.v == p1->v);
        }

        try
        {
          db->load (*p, p->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*p);
          assert (!p->s.loaded ());
          db->load (*p, p->s);

          assert (p->sv == o1.sv);
        }

        db->reload (o);
        assert (o.v == o1.v);
        assert (o.sv == o1.sv);
        t.commit ();
      }

      // Update changed section.
      //
      o1.sv[0]++;

      {
        transaction t (db->begin ());
        db->update (o1, o1.s);

        try
        {
          db->update (o, o.s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (o);
          db->update (o, o.s);
        }

        t.commit ();
      }
    }

    // Test reuse-inheritance, sections, and optimistic concurrency.
    //
    {
      using namespace test17;

      object o (123);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s1.loaded ());
        assert (o.s2.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s1.loaded ());
        assert (!p->s2.loaded ());
        assert (p->s1n != o.s1n && p->s2v != o.s2v);

        db->load (*p, p->s1);
        db->load (*p, p->s2);

        assert (p->s1.loaded ());
        assert (p->s2.loaded ());
        assert (p->s1n == o.s1n && p->s2v == o.s2v);

        t.commit ();
      }

      object o1 (o);

      // Update object.
      //
      for (unsigned short s (1); s < 3; ++s)
      {
        o1.s1n++;
        o1.s2v[0]++;

        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        db->update (o1);
        assert (o.v != o1.v);

        try
        {
          switch (s)
          {
          case 1: db->load (*p, p->s1); break;
          case 2: db->load (*p, p->s2); break;
          default: break;
          }
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*p);

          assert (!p->s1.loaded ());
          assert (!p->s2.loaded ());

          db->load (*p, p->s1);
          db->load (*p, p->s2);

          assert (p->s1n == o1.s1n && p->s2v == o1.s2v);
        }

        db->reload (o);
        assert (o.v == o1.v);
        assert (o.s1n == o1.s1n && o.s2v == o1.s2v);
        t.commit ();
      }

      // Update section.
      //
      for (unsigned short s (1); s < 3; ++s)
      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        switch (s)
        {
        case 1:
          o1.s1n++;
          db->update (o1, o1.s1);
          assert (o.v != o1.v);
          break;
        case 2:
          o1.s2v[0]++;
          db->update (o1, o1.s2);
          assert (o.v != o1.v);
          break;
        default: break;
        }

        try
        {
          switch (s)
          {
          case 1: db->load (*p, p->s1); break;
          case 2: db->load (*p, p->s2); break;
          default: break;
          }
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*p);

          assert (!p->s1.loaded ());
          assert (!p->s2.loaded ());

          db->load (*p, p->s1);
          db->load (*p, p->s2);

          assert (p->s1n == o1.s1n && p->s2v == o1.s2v);
        }

        db->reload (o);
        assert (o.v == o1.v);
        assert (o.s1n == o1.s1n && o.s2v == o1.s2v);
        t.commit ();
      }

      // Update changed section.
      //
      for (unsigned short s (1); s < 3; ++s)
      {
        transaction t (db->begin ());

        switch (s)
        {
        case 1:
          o1.s1n++;
          db->update (o1, o1.s1);
          break;
        case 2:
          o1.s2v[0]++;
          db->update (o1, o1.s2);
          break;
        default: break;
        }

        try
        {
          switch (s)
          {
          case 1: db->update (o, o.s1); break;
          case 2: db->update (o, o.s2); break;
          default: break;
          }
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (o);

          switch (s)
          {
          case 1: db->update (o, o.s1); break;
          case 2: db->update (o, o.s2); break;
          default: break;
          }
        }

        db->reload (o1);

        t.commit ();
      }
    }

    // Test change-updated section and change-tracking container.
    //
    {
      using namespace test18;

      object o (123);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        assert (o.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));

        assert (!p->s.loaded ());
        assert (p->sn != o.sn && p->sv != o.sv);

        db->load (*p, p->s);

        assert (p->s.loaded ());
        assert (p->sn == o.sn && p->sv == o.sv);

        t.commit ();
      }

      // Update but don't mark as changed.
      //
      o.sn++;
      o.sv.modify (0)++;

      {
        transaction t (db->begin ());
        db->update (o); // Automatically marked as changed.
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->sn == o.sn && p->sv == o.sv);
        t.commit ();
      }

      // Test updating just the section manually.
      //
      o.sn++;
      o.sv.modify (0)++;

      {
        transaction t (db->begin ());
        db->update (o, o.s);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> p (db->load<object> (o.id));
        db->load (*p, p->s);
        assert (p->sn == o.sn && p->sv == o.sv);
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
