// file      : common/const-member/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test const data members. The readonly test tests that const
// members are automatically treated as read-only.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
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

    // Const ids.
    //
    {
      const_id o (1);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->load<const_id> (1, o);
        t.commit ();
        assert (o.id == 1);
      }
    }

    {
      {
        const_auto_id o;
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
        assert (o.id == 1);
      }

      {
        transaction t (db->begin ());
        auto_ptr<const_auto_id> o (db->load<const_auto_id> (1));
        t.commit ();
        assert (o->id == 1);
      }
    }

    // Container.
    //
    {
      container o (1, 1);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<container> o (db->load<container> (1));
        t.commit ();

        assert (o->ccom.vec.size () == 1 && o->ccom.vec[0] == 1 &&
                o->ccom.cvec.size () == 1 && o->ccom.cvec[0] == 1 &&
                o->cvec.size () == 1 && o->cvec[0] == 1);
      }
    }

    // Wrapper.
    //
    {
      wrapper o (1, "abc", 1);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<wrapper> o (db->load<wrapper> (1));
        t.commit ();

        assert (*o->str == "abc" &&
                o->com->str == "abc" && o->com->num == 1 &&
                o->com->vec.size () == 1 && o->com->vec[0] == 1 &&
                o->vec->size () == 1 && (*o->vec)[0] == 1);
      }
    }

  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
