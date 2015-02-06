// file      : evolution/soft-delete/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test soft-delete functionality.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

#include <common/config.hxx> // DATABASE_XXX
#include <common/common.hxx>

#include "test2.hxx"
#include "test3.hxx"
#include "test2-odb.hxx"
#include "test3-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv, false));
    bool embedded (schema_catalog::exists (*db));

    // 1 - base version
    // 2 - migration
    // 3 - current version
    //
    unsigned short pass (*argv[argc - 1] - '0');

    switch (pass)
    {
    case 1:
      {
        using namespace v2;

        if (embedded)
        {
          // SQLite has broken foreign keys when it comes to DDL.
          //
#ifdef DATABASE_SQLITE
          db->connection ()->execute ("PRAGMA foreign_keys=OFF");
#endif
          transaction t (db->begin ());
          schema_catalog::drop_schema (*db);
          schema_catalog::create_schema (*db, "", false);
          schema_catalog::migrate_schema (*db, 2);
          t.commit ();

#ifdef DATABASE_SQLITE
          db->connection ()->execute ("PRAGMA foreign_keys=ON");
#endif
        }

        // Test soft-deleted objects.
        //
        {
          using namespace test1;

          object o (1);
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test basic soft-deleted member logic.
        //
        {
          using namespace test2;

          object o (1);
          o.str = "abc";
          o.num = 123;
          o.vec.push_back (123);
          o.ptr = new object1 (1);
          o.ptr->ptrs.push_back (&o);

          {
            transaction t (db->begin ());
            db->persist (o);
            db->persist (*o.ptr);
            t.commit ();
          }
        }

        // Test container with soft-deleted value member.
        //
        {
          using namespace test5;

          object o (1);
          o.vec.push_back (value ("abc", 123));

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test view with soft-deleted member.
        //
        {
          using namespace test6;

          object o (1);
          o.str = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted section member.
        //
        {
          using namespace test7;

          object o (1);
          o.str = "abc";
          o.num = 123;
          o.vec.push_back (123);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted members of a section.
        //
        {
          using namespace test8;

          object o (1);
          o.str = "abc";
          o.num = 123;
          o.vec.push_back (123);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test basic soft-deleted member logic in polymorphic classes.
        //
        {
          // We have to use v3 here because the discriminator includes
          // the namespace.
          //
          using namespace v3::test9;

          object o (1);
          o.bstr = "ab";
          o.dstr = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted section member in polymorphic classes.
        //
        {
          // We have to use v3 here because the discriminator includes
          // the namespace.
          //
          using namespace v3::test10;

          object o (1);
          o.bstr = "ab";
          o.dstr = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted members of a section in polymorphic classes.
        //
        {
          // We have to use v3 here because the discriminator includes
          // the namespace.
          //
          using namespace v3::test11;

          object o (1);
          o.bstr = "ab";
          o.dstr = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted member and optimistic concurrency.
        //
        {
          using namespace test12;

          object o (1);
          o.str = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted member in an object without id.
        //
        {
          using namespace test13;

          object o;
          o.str = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted member in an object with auto id.
        //
        {
          using namespace test14;

          object o;
          o.str = "abc";
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test summarily deleted composite values.
        //
        {
          using namespace test15;

          object o (1);
          o.v.reset (new value);
          o.v->str = "abc";
          o.v->vec.push_back (123);
          o.num = 123;

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted container member in a non-versioned object.
        //
        {
          using namespace test21;

          object o (1);
          o.num = 123;
          o.vec.push_back (123);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        // Test soft-deleted container member in a non-versioned section.
        //
        {
          using namespace test22;

          object o (1);
          o.num = 123;
          o.vec.push_back (123);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }
        }

        break;
      }
    case 2:
      {
        using namespace v3;

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_pre (*db, 3);
          t.commit ();
        }

        // Test soft-deleted objects.
        //
        {
          using namespace test1;

          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->num == 123);
            t.commit ();
          }
        }

        // Test basic soft-deleted member logic.
        //
        {
          using namespace test2;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->str == "abc" && p->num == 123 &&
                    p->vec[0] == 123 && p->ptr->id_ == 1);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc" &&
                                         query::ptr->id == 1));
            result::iterator i (r.begin ());
            assert (i != r.end () &&
                    i->str == "abc" && i->num == 123 &&
                    i->vec[0] == 123 && i->ptr->id_ == 1);
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);
          o.ptr = new object1 (2);
          o.ptr->ptrs.push_back (&o);

          {
            transaction t (db->begin ());
            db->persist (o);
            db->persist (*o.ptr);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "bcd" && p->num == 234 &&
                    p->vec[0] == 234 && p->ptr->id_ == 2);
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++;
          delete o.ptr;
          o.ptr = 0;

          {
            transaction t (db->begin ());
            db->erase<object1> (2);
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "bcde" && p->num == 235 &&
                    p->vec[0] == 235 && p->ptr == 0);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test container with soft-deleted value member.
        //
        {
          using namespace test5;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->vec[0].str == "abc" && p->vec[0].num == 123);
            t.commit ();
          }

          object o (2);
          o.vec.push_back (value ("bcd", 234));

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "bcd" && p->vec[0].num == 234);
            t.commit ();
          }

          o.vec.modify (0).str += 'e';
          o.vec.modify (0).num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "bcde" && p->vec[0].num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test view with soft-deleted member.
        //
        {
          using namespace test6;

          // All the database operations should still include the deleted
          // members.
          //
          {
            typedef odb::query<view> query;
            typedef odb::result<view> result;

            transaction t (db->begin ());
            result r (db->query<view> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "abc" && i->num == 123);
            t.commit ();
          }
        }

        // Test soft-deleted section member.
        //
        {
          using namespace test7;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            db->load (*p, p->s);
            assert (p->str == "abc" && p->num == 123 && p->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            assert (i->str == "abc" && i->num == 123 && i->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "bcd" && p->num == 234 && p->vec[0] == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++; // Automatically marks section as updated.

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "bcde" && p->num == 235 && p->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test soft-deleted members of a section.
        //
        {
          using namespace test8;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            db->load (*p, p->s);
            assert (p->str == "abc" && p->num == 123 && p->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            assert (i->str == "abc" && i->num == 123 && i->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "bcd" && p->num == 234 && p->vec[0] == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++; // Automatically marks section as updated.

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "bcde" && p->num == 235 && p->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test basic soft-deleted member logic in polymorphic classes.
        //
        {
          using namespace test9;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (static_cast<object*> (db->load<base> (1)));
            assert (p->bstr == "ab" && p->dstr == "abc" && p->num == 123);
            t.commit ();
          }

          {
            typedef odb::query<base> query;
            typedef odb::result<base> result;

            transaction t (db->begin ());
            result r (db->query<base> (query::bstr == "ab"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            object& o (static_cast<object&> (*i));
            assert (o.bstr == "ab" && o.dstr == "abc" && o.num == 123);
            t.commit ();
          }

          object o (2);
          o.bstr = "bc";
          o.dstr = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            assert (p->bstr == "bc" && p->dstr == "bcd" && p->num == 234);
            t.commit ();
          }

          o.bstr += 'd';
          o.dstr += 'e';
          o.num++;

          {
            transaction t (db->begin ());
            db->update (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            assert (p->bstr == "bcd" && p->dstr == "bcde" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (static_cast<base&> (o));
            t.commit ();
          }
        }

        // Test soft-deleted section member in polymorphic classes.
        //
        {
          using namespace test10;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<base> p (db->load<base> (1));
            db->load (*p, p->s);
            object& o (static_cast<object&> (*p));
            assert (o.bstr == "ab" && o.dstr == "abc" && o.num == 123);
            t.commit ();
          }

          {
            typedef odb::query<base> query;
            typedef odb::result<base> result;

            transaction t (db->begin ());
            result r (db->query<base> (query::bstr == "ab"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            object& o (static_cast<object&> (*i));
            assert (o.bstr == "ab" && o.dstr == "abc" && o.num == 123);
            t.commit ();
          }

          object o (2);
          o.bstr = "bc";
          o.dstr = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->bstr == "bc" && p->dstr == "bcd" && p->num == 234);
            t.commit ();
          }

          o.bstr += 'd';
          o.dstr += 'e';
          o.num++;
          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->bstr == "bcd" && p->dstr == "bcde" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (static_cast<base&> (o));
            t.commit ();
          }
        }

        // Test soft-deleted members of a section in polymorphic classes.
        //
        {
          using namespace test11;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<base> p (db->load<base> (1));
            db->load (*p, p->s);
            object& o (static_cast<object&> (*p));
            assert (o.bstr == "ab" && o.dstr == "abc" && o.num == 123);
            t.commit ();
          }

          {
            typedef odb::query<base> query;
            typedef odb::result<base> result;

            transaction t (db->begin ());
            result r (db->query<base> (query::bstr == "ab"));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            object& o (static_cast<object&> (*i));
            assert (o.bstr == "ab" && o.dstr == "abc" && o.num == 123);
            t.commit ();
          }

          object o (2);
          o.bstr = "bc";
          o.dstr = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->bstr == "bc" && p->dstr == "bcd" && p->num == 234);
            t.commit ();
          }

          o.bstr += 'd';
          o.dstr += 'e';
          o.num++;
          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->bstr == "bcd" && p->dstr == "bcde" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (static_cast<base&> (o));
            t.commit ();
          }
        }

        // Test soft-deleted member and optimistic concurrency.
        //
        {
          using namespace test12;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->str == "abc" && p->num == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "abc" && i->num == 123);
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "bcd" && p->num == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;

          {
            transaction t (db->begin ());
            unsigned long long v (o.v_);
            db->update (o);
            assert (o.v_ != v);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "bcde" && p->num == 235 && p->v_ == o.v_);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test soft-deleted member in an object without id.
        //
        {
          using namespace test13;

          typedef odb::query<object> query;
          typedef odb::result<object> result;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "abc" && i->num == 123);
            t.commit ();
          }

          object o;
          o.str = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);

            result r (db->query<object> (query::str == "bcd"));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "bcd" && i->num == 234);

            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase_query<object> (query::str == "bcd");
            t.commit ();
          }
        }

        // Test soft-deleted member in an object with auto id.
        //
        {
          using namespace test14;

          // All the database operations should still include the deleted
          // members.
          //
          unsigned long id;
          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "abc" && i->num == 123);
            id = i->id;
            t.commit ();
          }

          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (id));
            assert (p->str == "abc" && p->num == 123);
            t.commit ();
          }

          object o;
          o.str = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "bcd" && p->num == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "bcde" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test summarily deleted composite values.
        //
        {
          using namespace test15;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->v->str == "abc" && p->num == 123 &&
                    p->v->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::v.str == "abc"));
            result::iterator i (r.begin ());
            assert (i != r.end () &&
                    i->v->str == "abc" && i->num == 123 &&
                    i->v->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.v.reset (new value);
          o.v->str = "bcd";
          o.num = 234;
          o.v->vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->v->str == "bcd" && p->num == 234 &&
                    p->v->vec[0] == 234);
            t.commit ();
          }

          o.v->str += 'e';
          o.num++;
          o.v->vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->v->str == "bcde" && p->num == 235 &&
                    p->v->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test soft-deleted container member in a non-versioned object.
        //
        {
          using namespace test21;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->num == 123 && p->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->num == 123 && i->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 234 && p->vec[0] == 234);
            t.commit ();
          }

          o.num++;
          o.vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 235 && p->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        // Test soft-deleted container member in a non-versioned section.
        //
        {
          using namespace test22;

          // All the database operations should still include the deleted
          // members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            db->load (*p, p->s);
            assert (p->num == 123 && p->vec[0] == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            assert (i->num == 123 && i->vec[0] == 123);
            t.commit ();
          }

          object o (2);
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->num == 234 && p->vec[0] == 234);
            t.commit ();
          }

          o.num++;
          o.vec.modify (0)++; // Automatically marks section as changed.

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->num == 235 && p->vec[0] == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase (o);
            t.commit ();
          }
        }

        if (embedded)
        {
          transaction t (db->begin ());
          schema_catalog::migrate_schema_post (*db, 3);
          t.commit ();
        }
        break;
      }
    case 3:
      {
        using namespace v3;

        // Test soft-deleted objects.
        //
        {
          using namespace test1;

          try
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1)); // No such table.
            assert (false);
          }
          catch (const odb::exception&) {}
        }

        // Test basic soft-deleted member logic.
        //
        {
          using namespace test2;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->str == "" && p->num == 123 &&
                    p->vec.empty () && p->ptr == 0);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () &&
                    i->str == "" && i->num == 123 &&
                    i->vec.empty () && i->ptr == 0);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::str.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);
          o.ptr = new object1 (2);
          o.ptr->ptrs.push_back (&o);

          {
            transaction t (db->begin ());
            db->persist (o);
            db->persist (*o.ptr);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "" && p->num == 234 &&
                    p->vec.empty () && p->ptr == 0);
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++;
          delete o.ptr;
          o.ptr = 0;

          {
            transaction t (db->begin ());
            db->erase<object1> (2);
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "" && p->num == 235 &&
                    p->vec.empty () && p->ptr == 0);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test empty statement handling (INSERT, UPDATE).
        //
        {
          using namespace test3;

          // Now none of the database operations should include the
          // deleted member.
          //
          object o;
          o.str = "bcd";

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "");
            t.commit ();
          }

          o.str += 'e';

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "");
            t.commit ();
          }
        }

        // Test empty statement handling (SELECT in polymorphic loader).
        //
        {
          using namespace test4;

          // Now none of the database operations should include the
          // deleted member.
          //
          object o (1);
          o.str = "abc";

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<base> p (db->load<base> (1));
            assert (static_cast<object&> (*p).str == "");
            t.commit ();
          }
        }

        // Test container with soft-deleted value member.
        //
        {
          using namespace test5;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->vec[0].str == "" && p->vec[0].num == 123);
            t.commit ();
          }

          object o (2);
          o.vec.push_back (value ("bcd", 234));

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "" && p->vec[0].num == 234);
            t.commit ();
          }

          o.vec.modify (0).str += 'e';
          o.vec.modify (0).num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->vec[0].str == "" && p->vec[0].num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test view with soft-deleted member.
        //
        {
          using namespace test6;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            typedef odb::query<view> query;
            typedef odb::result<view> result;

            transaction t (db->begin ());
            result r (db->query<view> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "" && i->num == 123);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::str.is_null ())) == 1);
#endif
            t.commit ();
          }
        }

        // Test soft-deleted section member.
        //
        {
          using namespace test7;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->load (*p, p->s); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#endif
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++;
          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (o);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test soft-deleted members of a section.
        //
        {
          using namespace test8;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            db->load (*p, p->s);
            assert (p->str == "" && p->num == 123 && p->vec.empty ());
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            assert (i->str == "" && i->num == 123 && i->vec.empty ());

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::str.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "" && p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.str += 'e';
          o.num++;
          o.vec.modify (0)++; // No longer automatically marked as changed.

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "" && p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->str == "" && p->num == 235 && p->vec.empty ());
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test basic soft-deleted member logic in polymorphic classes.
        //
        {
          using namespace test9;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (static_cast<object*> (db->load<base> (1)));
            assert (p->bstr == "" && p->dstr == "" && p->num == 123);
            t.commit ();
          }

          {
            typedef odb::query<base> query;
            typedef odb::result<base> result;

            transaction t (db->begin ());
            result r (db->query<base> (query::id == 1));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            object& o (static_cast<object&> (*i));
            assert (o.bstr == "" && o.dstr == "" && o.num == 123);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<base> (query::bstr == "ab"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<base> (query::bstr.is_null ())) == 1);
#endif
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () &&
                    i->bstr == "" && i->dstr == "" && i->num);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::dstr == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::dstr.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o (2);
          o.bstr = "bc";
          o.dstr = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            assert (p->bstr == "" && p->dstr == "" && p->num == 234);
            t.commit ();
          }

          o.bstr += 'd';
          o.dstr += 'e';
          o.num++;

          {
            transaction t (db->begin ());
            db->update (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            assert (p->bstr == "" && p->dstr == "" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<base> (2);
            t.commit ();
          }
        }

        // Test soft-deleted section member in polymorphic classes.
        //
        {
          using namespace test10;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<base> p (db->load<base> (1));

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->load (*p, p->s); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#endif
            t.commit ();
          }

          object o (2);
          o.bstr = "bc";
          o.dstr = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (static_cast<base&> (o));
            t.commit ();
          }

          o.bstr += 'd';
          o.dstr += 'e';
          o.num++;
          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (static_cast<base&> (o));
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<base> (2);
            t.commit ();
          }
        }

        // Test soft-deleted members of a section in polymorphic classes.
        //
        {
          using namespace test11;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<base> p (db->load<base> (1));
            db->load (*p, p->s);
            object& o (static_cast<object&> (*p));
            assert (o.bstr == "" && o.dstr == "" && o.num == 123);
            t.commit ();
          }

          {
            typedef odb::query<base> query;
            typedef odb::result<base> result;

            transaction t (db->begin ());
            result r (db->query<base> (query::id == 1));
            result::iterator i (r.begin ());
            db->load (*i, i->s);
            assert (i != r.end ());
            object& o (static_cast<object&> (*i));
            assert (o.bstr == "" && o.dstr == "" && o.num == 123);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<base> (query::bstr == "ab"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<base> (query::bstr.is_null ())) == 1);
#endif
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            db->load (*i, i->s);
            assert (i != r.end () &&
                    i->bstr == "" && i->dstr == "" && i->num);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::dstr == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::dstr.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o (2);
          o.bstr = "bc";
          o.dstr = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->bstr == "" && p->dstr == "" && p->num == 234);
            t.commit ();
          }

          o.bstr += 'd';
          o.dstr += 'e';
          o.num++;
          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (static_cast<base&> (o));
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->bstr == "" && p->dstr == "" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<base> (2);
            t.commit ();
          }

          // Test empty statement detection in sections.
          //
          base b (3);
          b.bstr = "bc";

          {
            transaction t (db->begin ());
            db->persist (b);
            auto_ptr<base> p (db->load<base> (3));
            db->load (*p, p->s);
            assert (p->bstr == "");
            t.commit ();
          }

          b.bstr += 'd';
          b.s.change ();

          {
            transaction t (db->begin ());
            db->update (b);
            auto_ptr<base> p (db->load<base> (3));
            db->load (*p, p->s);
            assert (p->bstr == "");
            t.commit ();
          }
        }

        // Test soft-deleted member and optimistic concurrency.
        //
        {
          using namespace test12;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->str == "" && p->num == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "" && i->num == 123);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::str.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o (2);
          o.str = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "" && p->num == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;

          {
            transaction t (db->begin ());
            unsigned long long v (o.v_);
            db->update (o);
            assert (o.v_ != v);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->str == "" && p->num == 235 && p->v_ == o.v_);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test soft-deleted member in an object without id.
        //
        {
          using namespace test13;

          typedef odb::query<object> query;
          typedef odb::result<object> result;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "" && i->num == 123);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::str.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o;
          o.str = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);

            result r (db->query<object> (query::num == 234));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "" && i->num == 234);

            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase_query<object> (query::num == 234);
            t.commit ();
          }
        }

        // Test soft-deleted member in an object with auto id.
        //
        {
          using namespace test14;

          // Now none of the database operations should include the
          // deleted members.
          //
          unsigned long id;
          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->str == "" && i->num == 123);
            id = i->id;

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::str.is_null ())) == 1);
#endif
            t.commit ();
          }

          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (id));
            assert (p->str == "" && p->num == 123);
            t.commit ();
          }

          object o;
          o.str = "bcd";
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "" && p->num == 234);
            t.commit ();
          }

          o.str += 'e';
          o.num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (o.id));
            assert (p->str == "" && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (o.id);
            t.commit ();
          }
        }

        // Test summarily deleted composite values.
        //
        {
          using namespace test15;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->v.get () == 0 && p->num == 123);
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->v.get () == 0 && i->num == 123);

            // Logical delete in SQLite.
            //
#ifndef DATABASE_SQLITE
            try
            {
              db->query<object> (query::v.str == "abc"); // No such column.
              assert (false);
            }
            catch (const odb::exception&) {}
#else
            assert (size (db->query<object> (query::v.str.is_null ())) == 1);
#endif
            t.commit ();
          }

          object o (2);
          o.num = 234;

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->v.get () == 0 && p->num == 234);
            t.commit ();
          }

          o.num++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->v.get () == 0 && p->num == 235);
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test soft-deleted container member in a non-versioned object.
        //
        {
          using namespace test21;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            assert (p->num == 123 && p->vec.empty ());
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end () && i->num == 123 && i->vec.empty ());
            t.commit ();
          }

          object o (2);
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.num++;
          o.vec.modify (0)++;

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            assert (p->num == 235 && p->vec.empty ());
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        // Test soft-deleted container member in a non-versioned section.
        //
        {
          using namespace test22;

          // Now none of the database operations should include the
          // deleted members.
          //
          {
            transaction t (db->begin ());
            auto_ptr<object> p (db->load<object> (1));
            db->load (*p, p->s);
            assert (p->num == 123 && p->vec.empty ());
            t.commit ();
          }

          {
            typedef odb::query<object> query;
            typedef odb::result<object> result;

            transaction t (db->begin ());
            result r (db->query<object> (query::num == 123));
            result::iterator i (r.begin ());
            assert (i != r.end ());
            db->load (*i, i->s);
            assert (i->num == 123 && i->vec.empty ());
            t.commit ();
          }

          object o (2);
          o.num = 234;
          o.vec.push_back (234);

          {
            transaction t (db->begin ());
            db->persist (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.num++;
          o.vec.modify (0)++; // No longer automatically marks as changed.

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->num == 234 && p->vec.empty ());
            t.commit ();
          }

          o.s.change ();

          {
            transaction t (db->begin ());
            db->update (o);
            auto_ptr<object> p (db->load<object> (2));
            db->load (*p, p->s);
            assert (p->num == 235 && p->vec.empty ());
            t.commit ();
          }

          {
            transaction t (db->begin ());
            db->erase<object> (2);
            t.commit ();
          }
        }

        break;
      }
    default:
      {
        cerr << "unknown pass number '" << argv[argc - 1] << "'" << endl;
        return 1;
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
