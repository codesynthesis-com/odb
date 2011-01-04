// file      : common/container/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test container persistence.
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
using namespace odb;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    for (unsigned short i (0); i < 2; ++i)
    {
      object empty ("empty"), med ("medium"), full ("full");

      //
      // empty
      //

      empty.num = 0;
      empty.str = "";

      //
      // med
      //

      med.num = 999;
      med.str = "xxx";

      // vector
      //
      med.nv.push_back (123);
      med.nv.push_back (234);

      med.sv.push_back ("aaa");
      med.sv.push_back ("bbbb");

      med.cv.push_back (comp (123, "aaa"));
      med.cv.push_back (comp (234, "bbbb"));

      med.uv.push_back (123);
      med.uv.push_back (234);

      // list
      //
      med.sl.push_back ("aaa");
      med.sl.push_back ("bbbb");

      // set
      //
      med.ns.insert (123);
      med.ns.insert (234);

      med.ss.insert ("aaa");
      med.ss.insert ("bbbb");

      med.cs.insert (comp (123, "aaa"));
      med.cs.insert (comp (234, "bbbb"));

      // map
      //
      med.nsm[123] = "aaa";
      med.nsm[234] = "bbbb";

      med.snm["aaa"] = 123;
      med.snm["bbbb"] = 234;

      med.ncm[123] = comp (123, "aaa");
      med.ncm[234] = comp (234, "bbbb");

      med.csm[comp (123, "aaa")] = "aaa";
      med.csm[comp (234, "bbbb")] = "bbbb";

      //
      // full
      //

      full.num = 9999;
      full.str = "xxxx";

      // vector
      //
      full.nv.push_back (1234);
      full.nv.push_back (2345);
      full.nv.push_back (3456);

      full.sv.push_back ("aaaa");
      full.sv.push_back ("bbbbb");
      full.sv.push_back ("cccccc");

      full.cv.push_back (comp (1234, "aaaa"));
      full.cv.push_back (comp (2345, "bbbbb"));
      full.cv.push_back (comp (3456, "cccccc"));

      full.uv.push_back (1234);
      full.uv.push_back (2345);
      full.uv.push_back (3456);

      // list
      //
      full.sl.push_back ("aaaa");
      full.sl.push_back ("bbbbb");
      full.sl.push_back ("cccccc");

      // set
      //
      full.ns.insert (1234);
      full.ns.insert (2345);
      full.ns.insert (3456);

      full.ss.insert ("aaaa");
      full.ss.insert ("bbbbb");
      full.ss.insert ("cccccc");

      full.cs.insert (comp (1234, "aaaa"));
      full.cs.insert (comp (2345, "bbbbb"));
      full.cs.insert (comp (3456, "cccccc"));

      // map
      //
      full.nsm[1234] = "aaaa";
      full.nsm[2345] = "bbbbb";
      full.nsm[3456] = "cccccc";

      full.snm["aaaa"] = 1234;
      full.snm["bbbbb"] = 2345;
      full.snm["cccccc"] = 3456;

      full.ncm[1234] = comp (1234, "aaaa");
      full.ncm[2345] = comp (2345, "bbbbb");
      full.ncm[3456] = comp (3456, "cccccc");

      full.csm[comp (1234, "aaaa")] = "aaaa";
      full.csm[comp (2345, "bbbbb")] = "bbbbb";
      full.csm[comp (3456, "cccccc")] = "cccccc";

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (empty);
        db->persist (med);
        db->persist (full);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> e (db->load<object> ("empty"));
        auto_ptr<object> m (db->load<object> ("medium"));
        auto_ptr<object> f (db->load<object> ("full"));
        t.commit ();

        assert (empty == *e);
        assert (med == *m);
        assert (full == *f);
      }

      //
      // empty
      //

      empty.num = 99;
      empty.str = "xx";
      empty.nv.push_back (12);
      empty.sv.push_back ("aa");
      empty.cv.push_back (comp (12, "aa"));
      empty.uv.push_back (12);
      empty.sl.push_back ("aa");
      empty.ns.insert (12);
      empty.ss.insert ("aa");
      empty.cs.insert (comp (12, "aa"));
      empty.nsm[12] = "aa";
      empty.snm["aa"] = 12;
      empty.ncm[12] = comp (12, "aa");
      empty.csm[comp (12, "aa")] = "aa";

      //
      // med
      //

      med.num = 0;
      med.str = "";

      med.nv.clear ();
      med.sv.clear ();
      med.cv.clear ();
      med.uv.clear ();

      med.sl.clear ();

      med.ns.clear ();
      med.ss.clear ();
      med.cs.clear ();

      med.nsm.clear ();
      med.snm.clear ();
      med.ncm.clear ();
      med.csm.clear ();

      //
      // full
      //

      full.num++;
      full.str += "x";

      // vector
      //
      full.nv.back ()++;
      full.nv.push_back (4567);

      full.sv.back () += "c";
      full.sv.push_back ("ddddddd");

      full.cv.back ().num++;
      full.cv.back ().str += "c";
      full.cv.push_back (comp (4567, "ddddddd"));

      full.uv.back ()++;
      full.uv.push_back (4567);

      // list
      //
      full.sl.back () += "c";
      full.sl.push_back ("ddddddd");

      // set
      //
      full.ns.insert (4567);
      full.ss.insert ("ddddddd");
      full.cs.insert (comp (4567, "ddddddd"));

      // map
      //
      full.nsm[3456] += 'c';
      full.nsm[4567] = "ddddddd";

      full.snm["cccccc"]++;
      full.snm["ddddddd"] = 4567;

      full.ncm[3456].num++;
      full.ncm[3456].str += 'c';
      full.ncm[4567] = comp (4567, "ddddddd");

      full.csm[comp (3456, "cccccc")] += "c";
      full.csm[comp (4567, "ddddddd")] = "ddddddd";

      // update
      //
      {
        transaction t (db->begin ());
        db->update (empty);
        db->update (med);
        db->update (full);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> e (db->load<object> ("empty"));
        auto_ptr<object> m (db->load<object> ("medium"));
        auto_ptr<object> f (db->load<object> ("full"));
        t.commit ();

        assert (empty == *e);
        assert (med == *m);
        assert (full == *f);
      }

      // erase
      //
      if (i == 0)
      {
        transaction t (db->begin ());
        db->erase<object> ("empty");
        db->erase<object> ("medium");
        db->erase<object> ("full");
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
