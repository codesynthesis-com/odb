// file      : common/readonly/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test readonly members/objects. Also test that const members are
// treated as readonly. For other const member tests, see the const-
// member test.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/config.hxx> // DATABASE_*
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

    // Simple.
    //
    {
      simple o (1, 1);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      o.ro++;
      const_cast<unsigned long&> (o.co)++;
      o.rw++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->load<simple> (1, o);
        t.commit ();
      }

      assert (o.ro == 1 && o.co == 1 && o.rw == 2);
    }

    // Pointer.
    //
    {
      pointer p (1, new pointer (2));
      auto_ptr<pointer> p1 (new pointer (3));

      {
        transaction t (db->begin ());
        db->persist (p);
        db->persist (p.ro);
        db->persist (*p1);
        t.commit ();
      }

      delete p.ro;
      p.ro = p1.release ();
      const_cast<pointer*&> (p.co) = p.ro;
      p.rw = p.ro;

      {
        transaction t (db->begin ());
        db->update (p);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<pointer> p (db->load<pointer> (1));
        t.commit ();

        assert (p->ro->id == 2 && p->co->id == 2 && p->rw->id == 3);
      }
    }

    // Composite.
    //
    {
      composite o (1, 1);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      o.ro.v++;
      o.ro.ro++;
      const_cast<unsigned long&> (o.ro.co)++;
      o.ro.rw++;

      value& co (const_cast<value&> (o.co));
      co.v++;
      co.ro++;
      const_cast<unsigned long&> (co.co)++;
      co.rw++;

      o.rw.v++;
      o.rw.ro++;
      const_cast<unsigned long&> (o.rw.co)++;
      o.rw.rw++;

      o.v.v++;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->load<composite> (1, o);
        t.commit ();
      }

      assert (o.ro.v  == 1 &&
              o.ro.ro == 1 &&
              o.ro.co == 1 &&
              o.ro.rw == 1 &&

              o.co.v  == 1 &&
              o.co.ro == 1 &&
              o.co.co == 1 &&
              o.co.rw == 1 &&

              o.rw.v  == 1 &&
              o.rw.ro == 1 &&
              o.rw.co == 1 &&
              o.rw.rw == 2 &&

              o.v.v   == 1);
    }

    // Container.
    //
    {
      typedef vector<unsigned long> ulongs;

      container o (1);

      o.ro.push_back (1);
      o.ro.push_back (2);

      ulongs& co (const_cast<ulongs&> (o.co));
      co.push_back (1);
      co.push_back (2);

      o.rw.push_back (1);
      o.rw.push_back (2);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      o.ro[0]++;
      o.ro.pop_back ();

      co[0]++;
      co.pop_back ();

      o.rw[0]++;
      o.rw.pop_back ();

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->load<container> (1, o);
        t.commit ();
      }

      assert (o.ro.size () == 2 && o.ro[0] == 1 && o.ro[1] == 2 &&
              o.co.size () == 2 && o.co[0] == 1 && o.co[1] == 2 &&
              o.rw.size () == 1 && o.rw[0] == 2);
    }

    // Readonly object.
    //
    {
#ifndef DATABASE_COMMON
      typedef odb::object_traits_impl<simple_object, odb::id_common> so_traits;
      typedef odb::object_traits_impl<ro_object, odb::id_common> ro_traits;
      typedef odb::object_traits_impl<rw_object, odb::id_common> rw_traits;

      assert (so_traits::column_count ==
              so_traits::id_column_count + so_traits::readonly_column_count);

      assert (ro_traits::column_count ==
              ro_traits::id_column_count + ro_traits::readonly_column_count);

      assert (rw_traits::column_count !=
              rw_traits::id_column_count + rw_traits::readonly_column_count);
#endif

      simple_object so (1, 1);
      ro_object ro_o (1, 1);
      rw_object rw_o (1, 1);

      ro_o.cr.push_back (1);
      ro_o.cr.push_back (2);

      rw_o.cr.push_back (1);
      rw_o.cr.push_back (2);

      {
        transaction t (db->begin ());
        db->persist (so);
        db->persist (ro_o);
        db->persist (rw_o);
        t.commit ();
      }

      rw_o.sv++;
      rw_o.rw_sv++;

      {
        transaction t (db->begin ());
        //db->update (so);   // Compile error.
        //db->update (ro_o); // Compile error.
        db->update (rw_o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->load (1, so);
        db->load (1, ro_o);
        db->load (1, rw_o);
        t.commit ();
      }

      assert (rw_o.sv == 1 && rw_o.rw_sv == 2);
    }

    // Readonly object.
    //
    {
      wrapper o (1, 1);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      *o.pl = 2;
      *o.cpl = 2;
      o.pcl.reset (new unsigned long (2));
      const_cast<unsigned long&> (*o.cpcl) = 2;

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->load<wrapper> (1, o);
        t.commit ();
      }

      assert (*o.pl == 2 && *o.cpl == 2 && *o.pcl == 2 && *o.cpcl == 1);
    }

    // Readonly object with auto id.
    //
    {
      ro_auto o1 (1);
      ro_auto o2 (2);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<ro_auto> p1 (db->load<ro_auto> (o1.id));
        auto_ptr<ro_auto> p2 (db->load<ro_auto> (o2.id));
        t.commit ();

        assert (p1->num == o1.num);
        assert (p2->num == o2.num);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
