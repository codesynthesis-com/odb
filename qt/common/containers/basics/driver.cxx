// file      : qt/common/containers/basics/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test basic Qt containers persistence.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QCoreApplication>

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
  QCoreApplication app (argc, argv);

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

      med.nl.push_back (123);
      med.nl.push_back (234);

      med.cl.push_back (comp (123, "aaa"));
      med.cl.push_back (comp (234, "bbbb"));

      // linked list
      //
      med.sll.push_back ("aaa");
      med.sll.push_back ("bbbb");

      med.nll.push_back (123);
      med.nll.push_back (234);

      med.cll.push_back (comp (123, "aaa"));
      med.cll.push_back (comp (234, "bbbb"));

      // set
      //
      med.ns.insert (123);
      med.ns.insert (234);

      med.ss.insert ("aaa");
      med.ss.insert ("bbbb");

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

      // multimap
      //
      med.nsmm.insert (123, "aaa");
      med.nsmm.insert (123, "bbbb");
      med.nsmm.insert (234, "ccccc");

      med.snmm.insert ("aaa", 123);
      med.snmm.insert ("aaa", 234);
      med.snmm.insert ("bbb", 345);

      med.ncmm.insert (123, comp (123, "aaa"));
      med.ncmm.insert (123, comp (234, "bbbb"));
      med.ncmm.insert (234, comp (345, "ccccc"));

      // hash
      //
      med.nsh[123] = "aaa";
      med.nsh[234] = "bbbb";

      med.snh["aaa"] = 123;
      med.snh["bbb"] = 234;

      med.sch["iii"] = comp (123, "aaa");
      med.sch["jjj"] = comp (234, "bbbb");

      // multihash
      //
      med.nsmh.insert (123, "aaa");
      med.nsmh.insert (123, "bbbb");
      med.nsmh.insert (234, "ccccc");

      med.snmh.insert ("aaa", 123);
      med.snmh.insert ("aaa", 234);
      med.snmh.insert ("bbb", 345);

      med.ncmh.insert (123, comp (123, "aaa"));
      med.ncmh.insert (123, comp (234, "bbbb"));
      med.ncmh.insert (234, comp (345, "ccccc"));

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

      full.nl.push_back (1234);
      full.nl.push_back (2345);
      full.nl.push_back (3456);

      full.cl.push_back (comp (1234, "aaaa"));
      full.cl.push_back (comp (2345, "bbbbb"));
      full.cl.push_back (comp (3456, "cccccc"));

      // linked list
      //
      full.sll.push_back ("aaaa");
      full.sll.push_back ("bbbbb");
      full.sll.push_back ("cccccc");

      full.nll.push_back (1234);
      full.nll.push_back (2345);
      full.nll.push_back (3456);

      full.cll.push_back (comp (1234, "aaaa"));
      full.cll.push_back (comp (2345, "bbbbb"));
      full.cll.push_back (comp (3456, "cccccc"));

      // set
      //
      full.ns.insert (1234);
      full.ns.insert (2345);
      full.ns.insert (3456);

      full.ss.insert ("aaaa");
      full.ss.insert ("bbbbb");
      full.ss.insert ("cccccc");

      // map
      //
      full.nsm[1234] = "aaaa";
      full.nsm[2345] = "bbbbb";
      full.nsm[3456] = "cccccc";

      full.snm["aaaa"] = 1234;
      full.snm["bbbb"] = 2345;
      full.snm["cccc"] = 3456;

      full.ncm[1234] = comp (1234, "aaaa");
      full.ncm[2345] = comp (2345, "bbbbb");
      full.ncm[3456] = comp (3456, "cccccc");

      full.csm[comp (1234, "aaaa")] = "aaaa";
      full.csm[comp (2345, "bbbb")] = "bbbbb";
      full.csm[comp (3456, "cccc")] = "cccccc";

      // multimap
      //
      full.nsmm.insert (1234, "aaaa");
      full.nsmm.insert (1234, "bbbbb");
      full.nsmm.insert (2345, "cccccc");
      full.nsmm.insert (2345, "ddddddd");

      full.snmm.insert ("aaaa", 1234);
      full.snmm.insert ("aaaa", 2345);
      full.snmm.insert ("bbbb", 3456);
      full.snmm.insert ("bbbb", 4567);

      full.ncmm.insert (1234, comp (1234, "aaaa"));
      full.ncmm.insert (1234, comp (2345, "bbbbb"));
      full.ncmm.insert (2345, comp (3456, "cccccc"));
      full.ncmm.insert (2345, comp (4567, "ddddddd"));

      // hash
      //
      full.nsh[1234] = "aaaa";
      full.nsh[2345] = "bbbbb";
      full.nsh[3456] = "cccccc";

      full.snh["aaaa"] = 1234;
      full.snh["bbbb"] = 2345;
      full.snh["cccc"] = 3456;

      full.sch["iiii"] = comp (1234, "aaaa");
      full.sch["jjjj"] = comp (2345, "bbbbb");
      full.sch["kkkk"] = comp (3456, "cccccc");

      // multihash
      //
      full.nsmh.insert (1234, "aaaa");
      full.nsmh.insert (1234, "bbbbb");
      full.nsmh.insert (2345, "cccccc");
      full.nsmh.insert (2345, "ddddddd");

      full.snmh.insert ("aaaa", 1234);
      full.snmh.insert ("aaaa", 2345);
      full.snmh.insert ("bbbb", 3456);
      full.snmh.insert ("bbbb", 4567);

      full.ncmh.insert (1234, comp (1234, "aaaa"));
      full.ncmh.insert (1234, comp (2345, "bbbbb"));
      full.ncmh.insert (2345, comp (3456, "cccccc"));
      full.ncmh.insert (2345, comp (4567, "ddddddd"));

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

      // vector
      //
      empty.nv.push_back (12);
      empty.sv.push_back ("aa");
      empty.cv.push_back (comp (12, "aa"));
      empty.uv.push_back (12);

      // list
      //
      empty.sl.push_back ("aa");
      empty.nl.push_back (12);
      empty.cl.push_back (comp (12, "aa"));

      // linked list
      //
      empty.nll.push_back (12);
      empty.sll.push_back ("aa");
      empty.cll.push_back (comp (12, "aa"));

      // set
      //
      empty.ns.insert (12);
      empty.ss.insert ("aa");

      // map
      //
      empty.nsm[12] = "aa";
      empty.snm["aa"] = 12;
      empty.ncm[12] = comp (12, "aa");
      empty.csm[comp (12, "aa")] = "aa";

      // multimap
      //
      empty.nsmm.insert (12, "aa");
      empty.nsmm.insert (12, "bbb");
      empty.nsmm.insert (23, "cccc");
      empty.snmm.insert ("aa", 12);
      empty.snmm.insert ("aa", 23);
      empty.snmm.insert ("bb", 34);
      empty.ncmm.insert (12, comp (12, "aa"));
      empty.ncmm.insert (12, comp (23, "bb"));
      empty.ncmm.insert (23, comp (34, "cc"));

      // hash
      //
      empty.nsh[12] = "aa";
      empty.snh["aa"] = 12;
      empty.sch["ii"] = comp (12, "aa");

      // multihash
      //
      empty.nsmh.insert (12, "aa");
      empty.nsmh.insert (12, "bbb");
      empty.nsmh.insert (23, "cccc");
      empty.snmh.insert ("aa", 12);
      empty.snmh.insert ("aa", 23);
      empty.snmh.insert ("bb", 34);
      empty.ncmh.insert (12, comp (12, "aa"));
      empty.ncmh.insert (12, comp (23, "bb"));
      empty.ncmh.insert (23, comp (34, "cc"));

      //
      // med
      //

      med.num = 0;
      med.str = "";

      // vector
      //
      med.nv.clear ();
      med.sv.clear ();
      med.cv.clear ();
      med.uv.clear ();

      // list
      //
      med.sl.clear ();
      med.nl.clear ();
      med.cl.clear ();

      // linked list
      //
      med.nll.clear ();
      med.sll.clear ();
      med.cll.clear ();

      // set
      //
      med.ns.clear ();
      med.ss.clear ();

      // map
      //
      med.nsm.clear ();
      med.snm.clear ();
      med.ncm.clear ();
      med.csm.clear ();

      // multimap
      //
      med.nsmm.clear ();
      med.snmm.clear ();
      med.ncmm.clear ();

      // hash
      //
      med.nsh.clear ();
      med.snh.clear ();
      med.sch.clear ();

      // multihash
      //
      med.nsmh.clear ();
      med.snmh.clear ();
      med.ncmh.clear ();


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

      full.nl.back ()++;
      full.nl.push_back (4567);

      full.cl.back ().num++;
      full.cl.back ().str += "c";
      full.cl.push_back (comp (4567, "ddddddd"));

      // linked list
      //
      full.sll.back () += "c";
      full.sll.push_back ("ddddddd");

      full.nll.back ()++;
      full.nll.push_back (4567);

      full.cll.back ().num++;
      full.cll.back ().str += "c";
      full.cll.push_back (comp (4567, "ddddddd"));

      // set
      //
      full.ns.insert (4567);
      full.ss.insert ("ddddddd");

      // map
      //
      full.nsm[3456] += "c";
      full.nsm[4567] = "ddddddd";

      full.snm["cccc"]++;
      full.snm["dddd"] = 4567;

      full.ncm[3456].num++;
      full.ncm[3456].str += "c";
      full.ncm[4567] = comp (4567, "ddddddd");

      full.csm[comp (3456, "cccc")] += "c";
      full.csm[comp (4567, "dddd")] = "ddddddd";

      // multimap
      //
      full.nsmm.find (2345).value () += "d";
      full.nsmm.insert (3456, "eeeeeeee");

      full.snmm.find ("bbbb").value ()++;
      full.snmm.insert ("cccc", 5678);

      full.ncmm.find (1234).value ().num++;
      full.ncmm.find (2345).value ().str += "d";
      full.ncmm.insert (3456, comp (5678, "eeeeeeee"));

      // hash
      //
      full.nsh[3456] += "c";
      full.nsh[4567] = "ddddddd";

      full.snh["cccc"]++;
      full.snh["dddd"] = 4567;

      full.sch["iiii"].num++;
      full.sch["jjjj"].str += "b";
      full.sch["kkkk"] = comp (4567, "dddddddd");

      // multihash
      //
      full.nsmh.find (2345).value () += "d";
      full.nsmh.insert (3456, "eeeeeeee");

      full.snmh.find ("bbbb").value ()++;
      full.snmh.insert ("cccc", 5678);

      full.ncmh.find (1234).value ().num++;
      full.ncmh.find (2345).value ().str += "d";
      full.ncmh.insert (3456, comp (5678, "eeeeeeee"));

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
