// file      : common/const-object/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test database operations with const objects.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    aggr a (1);
    aggr ca_ (2); // o1 and o2 are NULL
    const aggr& ca (ca_);

    obj1* o1 (new obj1 (1));
    obj1* co1_ (new obj1 (2));
    const obj1* co1 (co1_);
    a.o1 = co1;

    auto_ptr<obj2> o2 (new obj2 (1));
    obj2* co2_ (new obj2 (2));
    a.o2.reset (co2_);
    auto_ptr<const obj2>& co2 (a.o2);

    // persist via references
    //
    {
      transaction t (db->begin ());
      db->persist (*o1);
      db->persist (*co1);
      db->persist (*o2);
      db->persist (*co2);
      db->persist (a);
      db->persist (ca);
      t.commit ();
    }

    // persist via pointers
    //
    o1->id += 2;
    co1_->id += 2;
    o2->id += 2;
    co2_->id += 2;

    {
      transaction t (db->begin ());
      db->persist (o1);
      db->persist (co1);
      db->persist (o2);
      db->persist (co2);
      t.commit ();
    }

    // load & compare
    //
    {
      transaction t (db->begin ());
      auto_ptr<aggr> a (db->load<aggr> (1));
      auto_ptr<const aggr> ca (db->load<aggr> (2));
      t.commit ();

      assert (a->o1->id == 2);
      assert (a->o2->id == 2);

      assert (ca->o1 == 0);
      assert (ca->o2.get () == 0);
    }

    // update via references
    //
    {
      transaction t (db->begin ());
      db->update (*o1);
      db->update (*co1);
      db->update (*o2);
      db->update (*co2);
      db->update (a);
      db->update (ca);
      t.commit ();
    }

    // update via pointers
    //
    {
      transaction t (db->begin ());
      db->update (o1);
      db->update (co1);
      db->update (o2);
      db->update (co2);
      t.commit ();
    }

    // query
    //
    typedef odb::query<obj1> query1;
    typedef odb::query<obj2> query2;

    typedef odb::result<const obj1> result1;
    typedef odb::result<const obj2> result2;

    {
      transaction t (db->begin ());
      result1 r1 (db->query<obj1> (query1::id < 3));
      // odb::result<obj1> ur (r1); // error
      size_t n1 (0);

      for (result1::iterator i (r1.begin ()); i != r1.end (); ++i)
      {
        // i->f (); // error
        i->cf ();
        // obj1* p (i.load ()); // error
        const obj1* p (i.load ());
        obj1 o (0);
        i.load (o);
        assert (p->id == o.id);
        delete p;
        n1++;
      }

      assert (n1 == 2);

      result2 r2 (db->query<obj2> (query2::id < 3));
      size_t n2 (0);

      for (result2::iterator i (r2.begin ()); i != r2.end (); ++i)
      {
        // i->f (); // error
        i->cf ();
        // auto_ptr<obj2> p (i.load ()); // error
        auto_ptr<const obj2> p (i.load ());
        obj2 o (0);
        i.load (o);
        assert (p->id == o.id);
        n2++;
      }

      assert (n2 == 2);

      t.commit ();
    }

    // erase via references
    //
    {
      transaction t (db->begin ());
      db->erase (*o1);
      db->erase (*co1);
      db->erase (*o2);
      db->erase (*co2);
      db->erase (a);
      db->erase (ca);
      t.commit ();
    }

    // erase via pointers
    //
    o1->id -= 2;
    co1_->id -= 2;
    o2->id -= 2;
    co2_->id -= 2;

    {
      transaction t (db->begin ());
      db->erase (o1);
      db->erase (co1);
      db->erase (o2);
      db->erase (co2);
      t.commit ();
    }

    // Test session and const/non-const object handling
    //
    {
      session s;
      transaction t (db->begin ());

      obj1 o1 (1);
      const obj1& co1 (o1);
      db->persist (co1);

      assert (db->load<obj1> (1) == &o1);

      t.commit ();
    }

    delete o1;
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
