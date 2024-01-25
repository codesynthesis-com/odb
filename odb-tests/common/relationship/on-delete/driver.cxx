// file      : common/relationship/on-delete/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test ON DELETE functionality.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
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

    object o;

    cascade c;
    c.p = &o;

    cascade_cont cc;
    cc.p.push_back (&o);

    set_null n;
    n.p = &o;

    set_null_cont nc;
    nc.p.push_back (&o);

    {
      transaction t (db->begin ());
      db->persist (o);
      db->persist (c);
      db->persist (cc);
      db->persist (n);
      db->persist (nc);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      db->erase (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      assert (db->find<cascade> (c.id) == 0);

      unique_ptr<cascade_cont> pcc (db->load<cascade_cont> (cc.id));
      assert (pcc->p.empty ());

      unique_ptr<set_null> pn (db->load<set_null> (n.id));
      assert (pn->p == 0);

      unique_ptr<set_null_cont> pnc (db->load<set_null_cont> (nc.id));
      assert (pnc->p.size () == 1 && pnc->p[0] == 0);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
