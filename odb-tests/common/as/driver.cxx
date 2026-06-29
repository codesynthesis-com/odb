// file      : common/as/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test C++ type mapping (#pragma map type as ...).
//

#include <memory>   // unique_ptr, shared_ptr, make_shared()
#include <vector>
#include <cstddef>  // size_t
#include <iostream>

#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/config.hxx> // MULTI_DATABASE
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

    // Test basic type mapping functionality.
    //
    {
      using namespace test1;

      object o1 (true, green, 123, 234);
      o1.m[false] = 123;
      o1.v.push_back (o1.ip);
      o1.cv.push_back (red);
      o1.cv.push_back (green);

      object o2 (false, blue, 234, 456);
      o2.m[true] = 234;
      o2.v.push_back (o2.ip);
      o2.cv.push_back (green);
      o2.cv.push_back (blue);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        using q = query<object>;

        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        object v1 (db->query_value<object> (q::b));
        object v2 (db->query_value<object> (q::c == blue));

        // The below query is not allowed and ends up with a compilation error
        // since the to() clause expression is not of a reference type.
        //
        //color c (blue);
        //object v3 (db->query_value<object> (q::c == q::_ref (c)));

        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
        assert (v1 == o1);
        assert (v2 == o2);
      }

      o1.b = false;
      o1.c = blue;
      o1.ip.first++;
      o1.ip.second--;
      o1.m[false]++;
      o1.m[true] = 234;
      o1.v.back () = o1.ip;
      o1.cv.modify_front () = green;
      o1.cv.push_back (red);

      o2.b = true;
      o2.c = red;
      o2.ip.first--;
      o2.ip.second++;
      o2.m[true]--;
      o2.m[false] = 345;
      o2.v.push_back (o2.ip);
      o2.cv.pop_back ();

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test wrapped simple type mapping.
    //
    {
      using namespace test2;

      object o1;
      o1.v.push_back (null_bool ());
      o1.v.push_back (false);

      object o2;
      o2.b = true;
      o2.v.push_back (true);
      o2.v.push_back (null_bool ());

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.b = false;
      o1.v[0] = true;
      o1.v[1].reset ();

      o2.b.reset ();
      o2.v.push_back (false);

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test wrapped composite type mapping.
    //
    {
      using namespace test3;

      object o1;
      o1.ip = intp (0, 0); // NULL
      o1.npv.push_back (o1.np);
      o1.ipv.push_back (o1.ip);

      object o2;
      o2.np = intp (123, 234);
      o1.ip = intp (234, 123);
      o2.npv.push_back (o2.np);
      o2.ipv.push_back (o2.ip);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.np = o1.npv[0] = intp (234, 456);
      o1.ip = o1.ipv.modify_at (0) = intp (456, 234);

      o2.np.reset ();
      o2.npv[0].reset ();
      o2.ip = o2.ipv.modify_at (0) = intp (0, 0); // NULL

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test id type mapping.
    //
    {
      using namespace test4;

      object o1 (123);
      o1.v.push_back (1);
      o1.v.push_back (2);
      o1.v.push_back (3);

      object o2 (234);
      o2.v.push_back (3);
      o2.v.push_back (2);
      o2.v.push_back (1);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }

      o1.i++;
      o1.v.pop_back ();
      o1.v.modify_front ()++;

      o2.i--;
      o2.v.clear ();
      o2.v.push_back (4);

      {
        transaction t (db->begin ());
        db->update (o1);
        db->update (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test version type mapping.
    //
    {
      using namespace test5;

      object o1 (100, 123);
      object o2 (200, 234);

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));

        assert (*p1 == o1);
        assert (*p2 == o2);

        p1->i--;
        p2->i++;

        db->update (*p1);
        db->update (*p2);

        t.commit ();
      }

      {
        transaction t (db->begin ());

        for (;;)
        {
          o1.i++;
          o2.i--;

          try
          {

            db->update (o1);
            db->update (o2);
            break;
          }
          catch (const odb::object_changed&)
          {
            db->reload (o1);
            db->reload (o2);
          }
        }

        t.commit ();
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> p1 (db->load<object> (o1.id));
        unique_ptr<object> p2 (db->load<object> (o2.id));
        t.commit ();

        assert (*p1 == o1);
        assert (*p2 == o2);
      }
    }

    // Test id type mapping, where the mapped and interface types are not
    // implicitly convertible to each other.
    //
    {
      using namespace test6;

      shared_ptr<obj1> dep (make_shared<obj1> (1));

      {
        transaction t (db->begin ());
        db->persist (dep);
        t.commit ();
      }

      // Simple interface value type.
      //
      {
        using namespace simple_intf;

        object o1 (1, 1, 1);
        object o2 (2, 1, 1);

        o1.note = string ("main");
        o1.usage = state::enabled;

        o2.dep = dependency (dep);

        shared_ptr<obj2> req (make_shared<obj2> (o1.id));
        o2.req = requirement (req);

        o2.source = o1.id;
        o2.source_usage = state::enabled;
        o2.source_distance = 3;

        {
          transaction t (db->begin ());
          db->persist (req);
          db->persist (o1);
          db->persist (o2);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          unique_ptr<object> p1 (db->load<object> (o1.id));
          unique_ptr<object> p2 (db->find<object> (o2.id));
          unique_ptr<object> p3 (db->find<object> (object_id (1000)));

          assert (*p1 == o1);
          assert (p2 != nullptr && *p2 == o2);
          assert (p3 == nullptr);

          using q = query<object>;

          // id == _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::id == q::_val (o1.id)));
            assert (v == o1);

            object_id id;
            q oq (q::id == q::_ref (id));
            id = o1.id;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // id != _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::id != q::_val (o1.id)));
            assert (v == o2);

            object_id id;
            q oq (q::id != q::_ref (id));
            id = o1.id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // id < _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::id < q::_val (o2.id)));
            assert (v == o1);

            object_id id;
            q oq (q::id < q::_ref (id));
            id = o2.id;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // id > _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::id > q::_val (o1.id)));
            assert (v == o2);

            object_id id;
            q oq (q::id > q::_ref (id));
            id = o1.id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // id <= _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::id <= q::_val (o1.id)));
            assert (v == o1);

            object_id id;
            q oq (q::id <= q::_ref (id));
            id = o1.id;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // id >= _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::id >= q::_val (o2.id)));
            assert (v == o2);

            object_id id;
            q oq (q::id >= q::_ref (id));
            id = o2.id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // id.in()
          //
          {
            object v2 (db->query_value<object> (q::id.in (o1.id, o1.id)));
            object v3 (db->query_value<object> (q::id.in (o1.id, o1.id, o1.id)));
            object v4 (db->query_value<object> (q::id.in (o1.id, o1.id, o1.id, o1.id)));
            object v5 (db->query_value<object> (q::id.in (o1.id, o1.id, o1.id, o1.id, o1.id)));

            assert (v2 == o1 && v3 == o1 && v4 == o1 && v5 == o1);
          }

          // id.in_range()
          //
          {
            std::vector<object_id> os ({o1.id});

            object v (
              db->query_value<object> (
                q::id.in_range (os.begin (), os.end ())));

            assert (v == o1);
          }

          // id.like()
          //
          {
            object v (db->query_value<object> (q::id.like (o1.id)));
            assert (v == o1);

            object_id id;
            q oq (q::id.like (q::_ref (id)));
            id = o1.id;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

#ifndef MULTI_DATABASE
          // id + '=' + _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                q::id + "=" + q::_val (o1.id.string ())));
            assert (v == o1);

            string s;
            q oq (q::id + "=" + q::_ref (s));
            s = o1.id.string ();
            object r (db->query_value<object> (oq));
            assert (r == o1);

            // Note: the following queries (use of the mapped types) are not
            // supported.
            //
            //object v (
            //  db->query_value<object> (
            //    q::id + "=" + q::_val (o1.id)));
            //
            //object v (
            //  db->query_value<object> (
            //    q::id + "=" + q::_ref (o1.id)));
          }
#endif

          // id == ref
          //
          {
            object o (db->query_value<object> (q::id == q::ref));
            assert (o == o1);
          }

          // ref == _val()/_ref()
          //
          {
            size_t n (0);
            for (const object& o: db->query<object> (q::ref == o1.id))
            {
              assert (o == o1 || o == o2);
              ++n;
            }
            assert (n == 2);

            object_id id;
            q oq (q::ref == q::_ref (id));
            id = o1.id;

            n = 0;
            for (const object& o: db->query<object> (oq))
            {
              assert (o == o1 || o == o2);
              ++n;
            }
            assert (n == 2);
          }

          // refs == _val()/_ref()
          //
          // @@ TMP Enable when GH issue #32 is fixed.
          //
#if 0
          {
            size_t n;

#ifndef MULTI_DATABASE
            object_id vids[1] = {o1.id};

            n = 0;
            for (const object& o:
                   db->query<object> (q::refs == q::_val (vids)))
            {
              assert (o == o1 || o == o2);
              ++n;
            }
            assert (n == 2);
#endif
            object_id rids[1];
            q oq (q::refs == q::_ref (rids));
            rids[0] = o1.id;

            n = 0;
            for (const object& o: db->query<object> (oq))
            {
              assert (o == o1 || o == o2);
              ++n;
            }
            assert (n == 2);
          }
#endif

          // next.id == _val()/_ref()
          //
          {
            object o (
              db->query_value<object> (
                q::next.id == o1.id && q::next.id != q::id));

            assert (o == o2);

            object_id id;
            q oq (q::next.id == q::_ref (id) && q::next.id != q::id);
            id = o1.id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // dep.obj == _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::dep.obj == dep->id));
            assert (v == o2);

            int id (0);
            q oq (q::dep.obj == q::_ref (id));
            id = dep->id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // req.obj == _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::req.obj == req->id));
            assert (v == o2);

            object_id id;
            q oq (q::req.obj == q::_ref (id));
            id = req->id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // note == _val()/_ref()
          //
          // Note that for the wrapper type, the query column parameters are
          // of the wrapped type (string rather than odb::nullable<string>).
          //
          {
            object v1 (db->query_value<object> (q::note == string ("main")));
            assert (v1 == o1);

            object v2 (db->query_value<object> (q::note.is_null ()));
            assert (v2 == o2);

            string s;
            q oq (q::note == q::_ref (s));
            s = "main";
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // usage == _val()
          //
          {
            object v (db->query_value<object> (q::usage == state::enabled));
            assert (v == o1);

            // The below query is not allowed and ends up with a compilation
            // error since the to() clause expression is not of a reference
            // type.
            //
            //state s (state::disabled);
            //q oq (q::usage == q::_ref (s));
            //s = state::enabled;
            //object r (db->query_value<object> (oq));
            //assert (r == o1);
          }

          // source == _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::source == o1.id));
            assert (v == o2);

            null_object_id id;
            q oq (q::source == q::_ref (id));
            id = null_object_id (o1.id);
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // source_usage == _val()
          //
          {
            object v1 (
              db->query_value<object> (q::source_usage == state::enabled));

            assert (v1 == o2);

            assert (
              db->query<object> (q::source_usage != state::enabled).empty ());

            object v2 (db->query_value<object> (q::source_usage.is_null ()));
            assert (v2 == o1);
          }

          // source_distance == _val()
          //
          {
            object v1 (
              db->query_value<object> (
                q::source_distance == o2.source_distance));

            assert (v1 == o2);

            assert (
              db->query<object> (
                q::source_distance != o2.source_distance).empty ());

            object v2 (db->query_value<object> (q::source_distance.is_null ()));
            assert (v2 == o1);
          }

          t.commit ();
        }
      }

      // Composite interface value type.
      //
      {
        using namespace composite_intf;

        object o1 (1, 2,  1, 2,  1, 2);
        object o2 (2, 3,  1, 2,  1, 2);

        o2.dep = dependency (dep);

        shared_ptr<obj2> req (make_shared<obj2> (o1.id));
        o2.req = requirement (req);

        o2.source = o1.id;

        {
          transaction t (db->begin ());
          db->persist (req);
          db->persist (o1);
          db->persist (o2);
          t.commit ();
        }

        {
          transaction t (db->begin ());
          unique_ptr<object> p1 (db->load<object> (o1.id));
          unique_ptr<object> p2 (db->find<object> (o2.id));
          unique_ptr<object> p3 (db->find<object> (object_id (1000, 1000)));

          assert (*p1 == o1);
          assert (p2 != nullptr && *p2 == o2);
          assert (p3 == nullptr);

          using q = query<object>;

          str_pair id1 (to_str_pair (o1.id));
          str_pair id2 (to_str_pair (o2.id));

          // id == _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (q::id.first  == q::_val (id1.first) &&
                                       q::id.second == q::_val (id1.second)));
            assert (v == o1);

            str_pair id;
            q oq (q::id.first == q::_ref (id.first) &&
                  q::id.second == q::_ref (id.second));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // id != _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (q::id.first  != q::_val (id1.first) ||
                                       q::id.second != q::_val (id1.second)));
            assert (v == o2);

            str_pair id;
            q oq (q::id.first != q::_ref (id.first) ||
                  q::id.second != q::_ref (id.second));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // id < _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                q::id.first < q::_val (id2.first) ||
                (q::id.first == q::_val (id2.first) &&
                 q::id.second < q::_val (id2.second))));
            assert (v == o1);

            str_pair id;
            q oq (q::id.first < q::_ref (id.first) ||
                  (q::id.first == q::_ref (id.first) &&
                   q::id.second < q::_ref (id.second)));
            id = id2;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // id > _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                q::id.first > q::_val (id1.first) ||
                (q::id.first == q::_val (id1.first) &&
                 q::id.second > q::_val (id1.second))));
            assert (v == o2);

            str_pair id;
            q oq (q::id.first > q::_ref (id.first) ||
                  (q::id.first == q::_ref (id.first) &&
                   q::id.second > q::_ref (id.second)));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // id <= _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                (q::id.first == q::_val (id1.first) &&    // ==
                 q::id.second == q::_val (id1.second)) ||
                (q::id.first < q::_val (id1.first) ||     // <
                 (q::id.first == q::_val (id1.first) &&
                  q::id.second < q::_val (id1.second)))));
            assert (v == o1);

            str_pair id;
            q oq ((q::id.first == q::_ref (id.first) &&    // ==
                   q::id.second == q::_ref (id.second)) ||
                  (q::id.first < q::_ref (id.first) ||     // <
                   (q::id.first == q::_ref (id.first) &&
                    q::id.second < q::_ref (id.second))));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

          // id >= _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                (q::id.first == q::_val (id2.first) &&    // ==
                 q::id.second == q::_val (id2.second)) ||
                (q::id.first > q::_val (id2.first) ||     // >
                 (q::id.first == q::_val (id2.first) &&
                  q::id.second > q::_val (id2.second)))));
            assert (v == o2);

            str_pair id;
            q oq ((q::id.first == q::_ref (id.first) &&    // ==
                   q::id.second == q::_ref (id.second)) ||
                  (q::id.first > q::_ref (id.first) ||     // >
                   (q::id.first == q::_ref (id.first) &&
                    q::id.second > q::_ref (id.second))));
            id = id2;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // id.in()
          //
          {
            object v2 (
              db->query_value<object> (
                q::id.first.in (id1.first, id1.first) &&
                q::id.second.in (id1.second, id1.second)));

            assert (v2 == o1);
          }

          // id.like()
          //
          {
            object v (
              db->query_value<object> (
                q::id.first.like (id1.first) &&
                q::id.second.like (id1.second)));
            assert (v == o1);

            str_pair id;
            q oq (q::id.first.like (q::_ref (id.first)) &&
                  q::id.second.like (q::_ref (id.second)));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }

#ifndef MULTI_DATABASE
          // id + '=' + _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                (q::id.first + "=" + q::_val (id1.first)) &&
                (q::id.second + "=" + q::_val (id1.second))));
            assert (v == o1);

            str_pair id;
            q oq ((q::id.first + "=" + q::_ref (id.first)) &&
                  (q::id.second + "=" + q::_ref (id.second)));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o1);
          }
#endif

          // id == ref
          //
          {
            object o (
              db->query_value<object> (
                q::id.first == q::ref.first &&
                q::id.second == q::ref.second));

            assert (o == o1);
          }

          // ref == _val()/_ref()
          //
          {
            size_t n (0);
            for (const object& o:
                   db->query<object> (q::ref.first == id1.first &&
                                      q::ref.second == id1.second))
            {
              assert (o == o1 || o == o2);
              ++n;
            }
            assert (n == 2);

            str_pair id;
            q oq (q::ref.first == q::_ref (id.first) &&
                  q::ref.second == q::_ref (id.second));
            id = id1;

            n = 0;
            for (const object& o: db->query<object> (oq))
            {
              assert (o == o1 || o == o2);
              ++n;
            }
            assert (n == 2);
          }

          // next.id == _val()/_ref()
          //
          {
            object o (
              db->query_value<object> (
                q::next.id.first == id1.first   &&
                q::next.id.second == id1.second &&
                (q::next.id.first != q::id.first ||
                 q::next.id.second != q::id.second)));

            assert (o == o2);

            str_pair id;
            q oq (q::next.id.first == q::_ref (id.first)   &&
                  q::next.id.second == q::_ref (id.second) &&
                  (q::next.id.first != q::id.first ||
                   q::next.id.second != q::id.second));

            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // dep.obj == _val()/_ref()
          //
          {
            object v (db->query_value<object> (q::dep.obj == dep->id));
            assert (v == o2);

            int id (0);
            q oq (q::dep.obj == q::_ref (id));
            id = dep->id;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // req.obj == _val()/_ref()
          //
          {
            object v (
              db->query_value<object> (
                q::req.obj.first == id1.first &&
                q::req.obj.second == id1.second));

            assert (v == o2);

            str_pair id;
            q oq (q::req.obj.first == q::_ref (id.first) &&
                  q::req.obj.second == q::_ref (id.second));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          // source == _val()/_ref()
          //
          {
            object v1 (
              db->query_value<object> (
                q::source.first == id1.first &&
                q::source.second == id1.second));

            assert (v1 == o2);

            object v2 (
              db->query_value<object> (
                q::source.first.is_null () &&
                q::source.second.is_null ()));

            assert (v2 == o1);

            str_pair id;
            q oq (q::source.first == q::_ref (id.first) &&
                  q::source.second == q::_ref (id.second));
            id = id1;
            object r (db->query_value<object> (oq));
            assert (r == o2);
          }

          t.commit ();
        }
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
