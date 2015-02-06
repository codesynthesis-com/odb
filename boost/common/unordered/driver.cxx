// file      : boost/common/unordered/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Boost unordered containers persistence.
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

      //
      // med
      //

      // set
      //
      med.ns.insert (123);
      med.ns.insert (234);

      med.ss.insert ("aaa");
      med.ss.insert ("bbbb");

      med.cms.insert (comp (123, "aaa"));
      med.cms.insert (comp (234, "bbbb"));

      // map
      //
      med.nsm[123] = "aaa";
      med.nsm[234] = "bbbb";

      med.snm["aaa"] = 123;
      med.snm["bbbb"] = 234;

      med.ncm[123] = comp (123, "aaa");
      med.ncm[234] = comp (234, "bbbb");

      med.csmm.insert (
        comp_str_multimap::value_type (comp (123, "aaa"), "aaa"));
      med.csmm.insert (
        comp_str_multimap::value_type (comp (234, "bbbb"), "bbbb"));

      //
      // full
      //

      // set
      //
      full.ns.insert (1234);
      full.ns.insert (2345);
      full.ns.insert (3456);

      full.ss.insert ("aaaa");
      full.ss.insert ("bbbbb");
      full.ss.insert ("cccccc");

      full.cms.insert (comp (1234, "aaaa"));
      full.cms.insert (comp (2345, "bbbbb"));
      full.cms.insert (comp (3456, "cccccc"));

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

      full.csmm.insert (
        comp_str_multimap::value_type (comp (1234, "aaaa"), "aaaa"));
      full.csmm.insert (
        comp_str_multimap::value_type (comp (2345, "bbbbb"), "bbbbb"));
      full.csmm.insert (
        comp_str_multimap::value_type (comp (3456, "cccccc"), "cccccc"));

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

      // empty
      //
      empty.ns.insert (12);
      empty.ss.insert ("aa");
      empty.cms.insert (comp (12, "aa"));

      empty.nsm[12] = "aa";
      empty.snm["aa"] = 12;
      empty.ncm[12] = comp (12, "aa");
      empty.csmm.insert (
        comp_str_multimap::value_type (comp (12, "aa"), "aa"));

      // med
      //
      med.ns.clear ();
      med.ss.clear ();
      med.cms.clear ();

      med.nsm.clear ();
      med.snm.clear ();
      med.ncm.clear ();
      med.csmm.clear ();

      // full
      //
      full.ns.insert (4567);
      full.ss.insert ("ddddddd");
      full.cms.insert (comp (4567, "ddddddd"));

      full.nsm[3456] += 'c';
      full.nsm[4567] = "ddddddd";
      full.snm["cccccc"]++;
      full.snm["ddddddd"] = 4567;
      full.ncm[3456].num++;
      full.ncm[3456].str += 'c';
      full.ncm[4567] = comp (4567, "ddddddd");
      full.csmm.find (comp (3456, "cccccc"))->second += "c";
      full.csmm.insert (
        comp_str_multimap::value_type (comp (4567, "ddddddd"), "ddddddd"));

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
