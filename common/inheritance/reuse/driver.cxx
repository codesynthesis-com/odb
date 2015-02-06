// file      : common/inheritance/reuse/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test reuse object inheritance.
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

    base b;
    b.comp_.bools.push_back (true);
    b.comp_.obools.push_back (true);
    b.comp_.num = 10;
    b.comp_.str = "comp bbb";
    b.comp_.nums.push_back (101);
    b.comp_.nums.push_back (102);
    b.comp_.onums.push_back (101);
    b.comp_.onums.push_back (102);
    b.num_ = 0;
    b.str_ = "bbb";
    b.strs_.push_back ("bbb one");
    b.strs_.push_back ("bbb two");
    b.ostrs_.push_back ("bbb one");
    b.ostrs_.push_back ("bbb two");

    object1 o1;
    o1.comp_.bools.push_back (false);
    o1.comp_.obools.push_back (false);
    o1.comp_.num = 11;
    o1.comp_.str = "comp o1o1o1";
    o1.comp_.nums.push_back (111);
    o1.comp_.nums.push_back (112);
    o1.comp_.onums.push_back (111);
    o1.comp_.onums.push_back (112);
    static_cast<base&> (o1).num_ = 1;
    o1.num1_ = 21;
    o1.str_ = "base o1o1o1";
    o1.strs_.push_back ("base o1o1o1 one");
    o1.strs_.push_back ("base o1o1o1 two");
    o1.ostrs_.push_back ("base o1o1o1 one");
    o1.ostrs_.push_back ("base o1o1o1 two");

    object2 o2;
    o2.comp_.bools.push_back (true);
    o2.comp_.bools.push_back (false);
    o2.comp_.obools.push_back (true);
    o2.comp_.obools.push_back (false);
    o2.comp_.num = 12;
    o2.comp_.str = "comp o2o2o2";
    o2.comp_.nums.push_back (121);
    o2.comp_.nums.push_back (122);
    o2.comp_.onums.push_back (121);
    o2.comp_.onums.push_back (122);
    o2.num_ = 2;
    static_cast<base&> (o2).str_ = "base o2o2o2";
    o2.str_ = "o2o2o2";
    o2.strs_.push_back ("base o2o2o2 one");
    o2.strs_.push_back ("base o2o2o2 two");
    o2.ostrs_.push_back ("base o2o2o2 one");
    o2.ostrs_.push_back ("base o2o2o2 two");

    object3 o3;
    o3.comp_.bools.push_back (false);
    o3.comp_.bools.push_back (false);
    o3.comp_.obools.push_back (false);
    o3.comp_.obools.push_back (false);
    o3.comp_.num = 13;
    o3.comp_.str = "comp o3o3o3";
    o3.comp_.nums.push_back (131);
    o3.comp_.nums.push_back (132);
    o3.comp_.onums.push_back (131);
    o3.comp_.onums.push_back (132);
    o3.num_ = 3;
    o3.str_ = "base o3o3o3";
    o3.strs_.push_back ("base o3o3o3 one");
    o3.strs_.push_back ("base o3o3o3 two");
    o3.ostrs_.push_back ("base o3o3o3 one");
    o3.ostrs_.push_back ("base o3o3o3 two");

    reference r;
    r.o1_ = &o1;

    empty e;
    e.comp_.bools.push_back (true);
    e.comp_.bools.push_back (true);
    e.comp_.obools.push_back (true);
    e.comp_.obools.push_back (true);
    e.comp_.num = 14;
    e.comp_.str = "comp eee";
    e.comp_.nums.push_back (141);
    e.comp_.nums.push_back (142);
    e.comp_.onums.push_back (141);
    e.comp_.onums.push_back (142);
    e.num_ = 4;
    e.str_ = "base eee";
    e.strs_.push_back ("base eee one");
    e.strs_.push_back ("base eee two");
    e.ostrs_.push_back ("base eee one");
    e.ostrs_.push_back ("base eee two");

    // persist
    //
    {
      transaction t (db->begin ());
      db->persist (b);
      db->persist (o1);
      db->persist (o2);
      db->persist (o3);
      db->persist (r);
      db->persist (e);
      t.commit ();
    }

    // load & check
    //
    {
      transaction t (db->begin ());
      auto_ptr<base> lb (db->load<base> (b.id_));
      auto_ptr<object1> lo1 (db->load<object1> (o1.id_));
      auto_ptr<object2> lo2 (db->load<object2> (o2.id_));
      auto_ptr<object3> lo3 (db->load<object3> (o3.id_));
      auto_ptr<empty> le (db->load<empty> (e.id_));
      auto_ptr<reference> lr (db->load<reference> (r.id_));
      t.commit ();

      assert (b == *lb);
      assert (o1 == *lo1);
      assert (o2 == *lo2);
      assert (o3 == *lo3);
      assert (lr->o1_->id_ == r.o1_->id_);
      assert (e == *le);

      delete lr->o1_;
    }

    // update
    //
    {
      transaction t (db->begin ());
      db->update (b);
      db->update (o1);
      db->update (o2);
      db->update (o3);
      db->update (r);
      db->update (e);
      t.commit ();
    }

    // query
    //
    {
      typedef odb::query<base> b_query;
      typedef odb::query<object1> o1_query;
      typedef odb::query<object2> o2_query;
      typedef odb::query<reference> r_query;

      typedef odb::result<reference> r_result;

      transaction t (db->begin ());

      assert (!db->query<base> (b_query::comp.num == 10).empty ());
      assert (!db->query<object1> (o1_query::num1 == 21).empty ());
      assert (!db->query<object2> (o2_query::num == 2).empty ());

      // Query condition with hidden members.
      //
      assert (
        !db->query<object2> (o2_query::base::str == "base o2o2o2").empty ());

      // Query condition with referenced composite member in base class.
      //
      {
        r_result r (db->query<reference> (r_query::o1->comp.num == 11));
        assert (!r.empty ());
        delete r.begin ()->o1_;
      }

      t.commit ();
    }

    // views
    //
    {
      typedef odb::query<object2_view> query;
      typedef odb::result<object2_view> result;

      transaction t (db->begin ());

      result r (db->query<object2_view> (query::num == o2.num_));
      result::iterator i (r.begin ());
      assert (i != r.end () &&
              i->num == o2.num_ && i->id == o2.id_ && i->str == o2.str_);
      assert (++i == r.end ());

      t.commit ();
    }

    // erase
    //
    {
      transaction t (db->begin ());
      db->erase (b);
      db->erase (o1);
      db->erase (o2);
      db->erase (o3);
      db->erase (r);
      db->erase (e);
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
