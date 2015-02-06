// file      : common/container/basics/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test basic container persistence.
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

    for (unsigned short i (0); i < 2; ++i)
    {
      object empty ("empty"), med ("medium"), full ("full");

      //
      // empty
      //

      empty.num = 0;
      empty.str = "";

#ifdef HAVE_CXX11
      // array
      //
      empty.na[0] = 123;
      empty.na[1] = 234;
      empty.na[2] = 345;

      empty.sa[0] = "aaa";
      empty.sa[1] = "bbbb";
      empty.sa[2] = "ccccc";

      empty.ca[0] = comp (123, "aaa");
      empty.ca[1] = comp (234, "bbbb");
      empty.ca[2] = comp (345, "ccccc");
#endif


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

      // deque
      //
      med.nd.push_back (123);
      med.nd.push_back (234);

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

#ifdef HAVE_CXX11
      // array
      //
      med.na[0] = 123;
      med.na[1] = 234;
      med.na[2] = 345;

      med.sa[0] = "aaa";
      med.sa[1] = "bbbb";
      med.sa[2] = "ccccc";

      med.ca[0] = comp (123, "aaa");
      med.ca[1] = comp (234, "bbbb");
      med.ca[2] = comp (345, "ccccc");

      // forward_list
      //
      med.nfl.push_front (234);
      med.nfl.push_front (123);

      med.sfl.push_front ("bbbb");
      med.sfl.push_front ("aaa");

      med.cfl.push_front (comp (234, "bbbb"));
      med.cfl.push_front (comp (123, "aaa"));

      // unordered_set
      //
      med.nus.insert (123);
      med.nus.insert (234);

      med.sus.insert ("aaa");
      med.sus.insert ("bbbb");

      med.cus.insert (comp (123, "aaa"));
      med.cus.insert (comp (234, "bbbb"));

      // unordered_map
      //
      med.nsum[123] = "aaa";
      med.nsum[234] = "bbbb";

      med.snum["aaa"] = 123;
      med.snum["bbbb"] = 234;

      med.ncum[123] = comp (123, "aaa");
      med.ncum[234] = comp (234, "bbbb");

      med.csum[comp (123, "aaa")] = "aaa";
      med.csum[comp (234, "bbbb")] = "bbbb";
#endif

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

      // deque
      //
      full.nd.push_back (1234);
      full.nd.push_back (2345);
      full.nd.push_back (3456);

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

#ifdef HAVE_CXX11
      // array
      //
      full.na[0] = 123;
      full.na[1] = 234;
      full.na[2] = 345;

      full.sa[0] = "aaa";
      full.sa[1] = "bbbb";
      full.sa[2] = "ccccc";

      full.ca[0] = comp (123, "aaa");
      full.ca[1] = comp (234, "bbbb");
      full.ca[2] = comp (345, "ccccc");

      // forward_list
      //
      full.nfl.push_front (345);
      full.nfl.push_front (234);
      full.nfl.push_front (123);

      full.sfl.push_front ("ccccc");
      full.sfl.push_front ("bbbb");
      full.sfl.push_front ("aaa");

      full.cfl.push_front (comp (345, "ccccc"));
      full.cfl.push_front (comp (234, "bbbb"));
      full.cfl.push_front (comp (123, "aaa"));

      // unordered_set
      //
      full.nus.insert (1234);
      full.nus.insert (2345);
      full.nus.insert (3456);

      full.sus.insert ("aaaa");
      full.sus.insert ("bbbbb");
      full.sus.insert ("cccccc");

      full.cus.insert (comp (1234, "aaaa"));
      full.cus.insert (comp (2345, "bbbbb"));
      full.cus.insert (comp (3456, "cccccc"));

      // unordered_map
      //
      full.nsum[1234] = "aaaa";
      full.nsum[2345] = "bbbbb";
      full.nsum[3456] = "cccccc";

      full.snum["aaaa"] = 1234;
      full.snum["bbbbb"] = 2345;
      full.snum["cccccc"] = 3456;

      full.ncum[1234] = comp (1234, "aaaa");
      full.ncum[2345] = comp (2345, "bbbbb");
      full.ncum[3456] = comp (3456, "cccccc");

      full.csum[comp (1234, "aaaa")] = "aaaa";
      full.csum[comp (2345, "bbbbb")] = "bbbbb";
      full.csum[comp (3456, "cccccc")] = "cccccc";
#endif

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
      empty.nd.push_back (12);

      empty.ns.insert (12);
      empty.ss.insert ("aa");
      empty.cs.insert (comp (12, "aa"));

      empty.nsm[12] = "aa";
      empty.snm["aa"] = 12;
      empty.ncm[12] = comp (12, "aa");
      empty.csm[comp (12, "aa")] = "aa";

#ifdef HAVE_CXX11
      empty.nfl.push_front (12);
      empty.sfl.push_front ("aa");
      empty.cfl.push_front (comp (12, "aa"));

      empty.nus.insert (12);
      empty.sus.insert ("aa");
      empty.cus.insert (comp (12, "aa"));

      empty.nsum[12] = "aa";
      empty.snum["aa"] = 12;
      empty.ncum[12] = comp (12, "aa");
      empty.csum[comp (12, "aa")] = "aa";
#endif

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

      med.nd.clear ();

      med.ns.clear ();
      med.ss.clear ();
      med.cs.clear ();

      med.nsm.clear ();
      med.snm.clear ();
      med.ncm.clear ();
      med.csm.clear ();

#ifdef HAVE_CXX11
      med.nfl.clear ();
      med.sfl.clear ();
      med.cfl.clear ();

      med.nus.clear ();
      med.sus.clear ();
      med.cus.clear ();

      med.nsum.clear ();
      med.snum.clear ();
      med.ncum.clear ();
      med.csum.clear ();
#endif

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

      // deque
      //
      full.nd.push_front (456);

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

#ifdef HAVE_CXX11
      // array
      //
      full.na[0]++;
      full.sa[0] += 'a';
      full.ca[0].num++;
      full.ca[0].str += 'a';

      // forward_list
      //
      full.nfl.front ()++;
      full.nfl.push_front (4567);

      full.sfl.front () += 'a';
      full.sfl.push_front ("ddddddd");

      full.cfl.front ().num++;
      full.cfl.front ().str += 'a';
      full.cfl.push_front (comp (4567, "ddddddd"));

      // unordered_set
      //
      full.nus.insert (4567);
      full.sus.insert ("ddddddd1"); // 1 is to preserve order in VC++ 10.
      full.cus.insert (comp (4567, "ddddddd1"));

      // unordered_map
      //
      full.nsum[3456] += 'c';
      full.nsum[4567] = "ddddddd";

      full.snum["cccccc"]++;
      full.snum["ddddddd1"] = 4567;

      full.ncum[3456].num++;
      full.ncum[3456].str += 'c';
      full.ncum[4567] = comp (4567, "ddddddd");

      full.csum[comp (3456, "cccccc")] += "c";
      full.csum[comp (4567, "ddddddd1")] = "ddddddd";
#endif

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
