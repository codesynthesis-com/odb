// file      : common/session/custom/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test custom session (C++11 only).
//

#include <memory>
#include <cstddef> // std::size_t
#include <cassert>
#include <iostream>

#include <odb/tracer.hxx>
#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>
#include <odb/details/config.hxx> // ODB_CXX11_*

#include <common/common.hxx>

#include "session.hxx"

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;

using odb::database;
using odb::transaction;

struct counting_tracer: odb::tracer
{
  virtual void
  execute (odb::connection&, const char*) {count++;}
  size_t count;
};

static counting_tracer tracer;

struct failed {};

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Simple Tech Ltd.
    //
    {
      shared_ptr<employer> er (new employer ("Simple Tech Ltd", "ST"));

      shared_ptr<employee> john (new employee ("John", "Doe", er));
      shared_ptr<employee> jane (new employee ("Jane", "Doe", er));

      transaction t (db->begin ());

      db->persist (er);
      db->persist (john);
      db->persist (jane);

      t.commit ();
    }

    // Complex Systems Inc.
    //
    {
      shared_ptr<employer> er (new employer ("Complex Systems Inc", "CS"));

      shared_ptr<employee> john (new employee ("John", "Smith", er));
      shared_ptr<employee> jane (new employee ("Jane", "Smith", er));

      transaction t (db->begin ());

      db->persist (er);
      db->persist (john);
      db->persist (jane);

      t.commit ();
    }

    {
      session s;
      shared_ptr<employer> st, cs;
      shared_ptr<employee> ste, cse;

      {
        transaction t (db->begin ());

        st = db->load<employer> ("Simple Tech Ltd");
#ifdef ODB_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGUMENT
        ste = db->load<employee> (st->employees ()[0].object_id ());
#else
        ste = db->load<employee> (st->employees ()[0].object_id<employee> ());
#endif

        // Test object cache.
        //
        shared_ptr<employee> e (st->employees ()[0].load ());
        assert (ste->employer () == st);
        assert (ste == e);

        t.commit ();
      }

      {
        transaction t (db->begin ());

        cs = db->load<employer> ("Complex Systems Inc");
#ifdef ODB_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGUMENT
        cse = db->load<employee> (cs->employees ()[0].object_id ());
#else
        cse = db->load<employee> (cs->employees ()[0].object_id<employee> ());
#endif
        cs->employees ()[0].load ();

        t.commit ();
      }

      cs->symbol ("CSI");

      // Swap employees.
      //
      ste->employer (cs);
      cse->employer (st);
      st->employees ()[0] = cse;
      cs->employees ()[0] = ste;

      {
        transaction t (db->begin ());
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 3);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 0);
        t.commit ();
      }

      cs->symbol ("COMP");
      st->symbol ("SMPL");

      {
        transaction t (db->begin ());
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 0);
        t.commit ();
      }

      // Explicit update.
      //
      cs->symbol ("CS");
      st->symbol ("ST");

      {
        transaction t (db->begin ());
        db->update (cs);
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 1);
        t.commit ();
      }

      // Rollback after update.
      //
      cs->symbol ("CSI");

      try
      {
        transaction t (db->begin ());
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 1);
        throw failed ();
        t.commit ();
      }
      catch (const failed&)
      {
        transaction t (db->begin ());
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 1);
        t.commit ();
      }
    }

    // Test session destruction before transaction is commited.
    //
    {
      transaction t (db->begin ());
      {
        session s;
        shared_ptr<employer> st (db->load<employer> ("Simple Tech Ltd"));
        st->symbol ("STL");
        tracer.count = 0;
        t.tracer (tracer);
        s.flush (*db);
        assert (tracer.count == 1);
      }
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
