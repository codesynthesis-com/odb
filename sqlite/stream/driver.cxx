// file      : sqlite/stream/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test SQLite BLOB/TEXT incremental I/O.
//

#include <vector>
#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <odb/sqlite/text-stream.hxx>
#include <odb/sqlite/blob-stream.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace sqlite = odb::sqlite;
using namespace sqlite;

template <typename S>
void
print (const S&)
{
  /*
  cerr << s.db () << '.'
       << s.table () << '.'
       << s.column () << '#'
       << s.rowid () << endl;
  */
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    string txt (1024 * 1024, 't');
    vector<char> blb (1024 * 1024, 'b');

    object o;

    {
      transaction tx (db->begin ());

      o.t.size (txt.size ());
      o.b.size (blb.size ());
      o.bv.push_back (blob (blb.size ()));
      o.bv.push_back (blob (blb.size ()));

      db->persist (o);

      print (o.t);
      print (o.b);
      print (o.bv[0]);
      print (o.bv[1]);

      blob_stream bs (o.b, true);
      bs.write (blb.data (), blb.size ());

      text_stream ts (o.t, true);
      ts.write (txt.data (), txt.size ());

      for (vector<blob>::iterator i (o.bv.begin ()); i != o.bv.end (); ++i)
      {
        blob_stream bs (*i, true);
        bs.write (blb.data (), blb.size ());
      }

      tx.commit ();
    }

    {
      transaction tx (db->begin ());
      auto_ptr<object> p (db->load<object> (o.id));

      print (p->t);
      print (p->b);
      print (p->bv[0]);
      print (p->bv[1]);

      text_stream ts (p->t, false);
      string t (ts.size (), '*');
      ts.read (const_cast<char*> (t.c_str ()), t.size ());
      assert (t == txt);

      blob_stream bs (p->b, false);
      vector<char> b (bs.size (), '\0');
      bs.read (b.data (), b.size ());
      assert (b == blb);

      for (vector<blob>::iterator i (p->bv.begin ()); i != p->bv.end (); ++i)
      {
        blob_stream bs (*i, false);
        vector<char> b (bs.size (), '\0');
        bs.read (b.data (), b.size ());
        assert (b == blb);
      }

      assert (p->nb.null ());

      tx.commit ();
    }

    txt.resize (txt.size () + 1, 't');
    txt[0] = 'A';
    txt[txt.size () - 1] = 'Z';

    blb.resize (blb.size () - 1);
    blb.front () = 'A';
    blb.back () = 'Z';

    {
      transaction tx (db->begin ());

      o.t.clear ();
      o.t.size (txt.size ());

      o.b.clear ();
      o.b.size (blb.size ());

      o.bv[0].clear ();
      o.bv[0].size (blb.size ());

      o.bv[1].clear ();
      o.bv[1].size (blb.size ());

      o.nb = blob (blb.size ());

      db->update (o);

      print (o.t);
      print (o.b);
      print (o.bv[0]);
      print (o.bv[1]);
      print (*o.nb);

      {
        text_stream ts (o.t, true);
        ts.write (txt.data (), txt.size ());
      }

      {
        blob_stream bs (o.b, true);
        bs.write (blb.data (), blb.size ());
      }

      for (vector<blob>::iterator i (o.bv.begin ()); i != o.bv.end (); ++i)
      {
        blob_stream bs (*i, true);
        bs.write (blb.data (), blb.size ());
      }

      {
        blob_stream bs (*o.nb, true);
        bs.write (blb.data (), blb.size ());
      }

      tx.commit ();
    }

    {
      transaction tx (db->begin ());
      auto_ptr<object> p (db->load<object> (o.id));

      print (p->t);
      print (p->b);
      print (p->bv[0]);
      print (p->bv[1]);
      print (*p->nb);

      text_stream ts (p->t, false);
      string t (ts.size (), '*');
      ts.read (const_cast<char*> (t.c_str ()), t.size ());
      assert (t == txt);

      blob_stream bs (p->b, false);
      vector<char> b (bs.size (), '\0');
      bs.read (b.data (), b.size ());
      assert (b == blb);

      for (vector<blob>::iterator i (p->bv.begin ()); i != p->bv.end (); ++i)
      {
        blob_stream bs (*i, false);
        vector<char> b (bs.size (), '\0');
        bs.read (b.data (), b.size ());
        assert (b == blb);
      }

      {
        blob_stream bs (*p->nb, false);
        vector<char> b (bs.size (), '\0');
        bs.read (b.data (), b.size ());
        assert (b == blb);
      }

      tx.commit ();
    }

    // Test query.
    //

    txt.resize (32);
    blb.resize (64);

    {
      transaction tx (db->begin ());

      o.t.size (txt.size ());
      o.b.size (blb.size ());
      o.bv.clear ();
      o.nb.reset ();

      db->update (o);

      text_stream ts (o.t, true);
      ts.write (txt.data (), txt.size ());

      blob_stream bs (o.b, true);
      bs.write (blb.data (), blb.size ());

      tx.commit ();
    }

    {
      typedef sqlite::query<object> query;
      transaction tx (db->begin ());

      {
        object o1 (db->query_value<object> (query::t == txt));

        blob_stream bs (o1.b, false);
        vector<char> b (bs.size (), '\0');
        bs.read (b.data (), b.size ());
        assert (b == blb);
      }

      {
        object o1 (db->query_value<object> (query::b == blb));

        text_stream ts (o1.t, false);
        string t (ts.size (), '*');
        ts.read (const_cast<char*> (t.c_str ()), t.size ());
        assert (t == txt);
      }

      tx.commit ();
    }

    // Test view.
    //
    {
      typedef sqlite::query<view> query;
      transaction tx (db->begin ());

      view v (db->query_value<view> (query::t == txt));

      blob_stream bs (v.b, false);
      vector<char> b (bs.size (), '\0');
      bs.read (b.data (), b.size ());
      assert (b == blb);

      tx.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
