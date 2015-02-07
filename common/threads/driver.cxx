// file      : common/threads/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test operations in a multi-threaded environment.
//

#include <vector>
#include <memory>   // std::auto_ptr
#include <cstddef>  // std::size_t
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <odb/details/shared-ptr.hxx>
#include <odb/details/thread.hxx>

#include <common/config.hxx> // DATABASE_*
#include <common/common.hxx>

#if defined(DATABASE_SQLITE) || defined(DATABASE_COMMON)
#  include <odb/sqlite/database.hxx>
#endif

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;
namespace details = odb::details;

const unsigned long thread_count = 32;
const unsigned long iteration_count = 50;
const unsigned long sub_iteration_count = 20;

struct task
{
  task (database& db, unsigned long n)
      : db_ (db), n_ (n)
  {
  }

  void*
  execute ()
  {
    try
    {
      for (unsigned long i (0); i < iteration_count; ++i)
      {
        unsigned long id ((n_ * iteration_count + i) * 3);

        {
          object o1 (id, "first object");
          object o2 (id + 1, "second object");
          object o3 (id + 2, "third object");

          transaction t (db_.begin ());
          db_.persist (o1);
          db_.persist (o2);
          db_.persist (o3);
          t.commit ();
        }

        // The following transaction may lead to a deadlock in some database
        // implementations (read to write lock upgrade).
        //
        while (true)
        {
          try
          {
#if !defined(DATABASE_SQLITE) && !defined(DATABASE_COMMON)
            transaction t (db_.begin ());
#else
            // SQLite has a peculiar table locking mode (shared cache)
            // which can lead to any of the transactions in this test
            // deadlocking even though they shouldn't from the user's
            // perspective. One way to work around this problem is to
            // start a "write" transaction as such right away.
            //
            transaction t;

            if (db_.id () != odb::id_sqlite)
              t.reset (db_.begin ());
            else
            {
              t.reset (
                static_cast<odb::sqlite::database&> (db_).begin_immediate ());
            }
#endif
            auto_ptr<object> o (db_.load<object> (id));
            assert (o->str_ == "first object");
            o->str_ = "another value";
            db_.update (*o);
            t.commit ();
            break;
          }
          catch (const deadlock&) {}
        }

        for (unsigned long j (0); j < sub_iteration_count; ++j)
        {
          typedef odb::query<object> query;
          typedef odb::prepared_query<object> prep_query;
          typedef odb::result<object> result;

          transaction t (db_.begin ());

          prep_query pq (db_.lookup_query<object> ("object-query"));

          if (!pq)
          {
            pq = db_.prepare_query<object> (
              "object-query", query::str == "another value");
            db_.cache_query (pq);
          }

          result r (pq.execute (false));

          bool found (false);
          for (result::iterator i (r.begin ()); i != r.end (); ++i)
          {
            if (i->id_ == id)
            {
              found = true;
              break;
            }
          }
          assert (found);

          t.commit ();
        }

        {
          transaction t (db_.begin ());
          db_.erase<object> (id);
          t.commit ();
        }
      }
    }
    catch (const odb::exception& e)
    {
      cerr << e.what () << endl;
      return reinterpret_cast<void*> (1);
    }

    return 0;
  }

  static void*
  execute (void* arg)
  {
    return static_cast<task*> (arg)->execute ();
  }

  database& db_;
  unsigned long n_;
};

bool
test (int argc, char* argv[], size_t max_connections)
{
  auto_ptr<database> db (create_database (argc, argv, true, max_connections));

  vector<details::shared_ptr<details::thread> > threads;
  vector<details::shared_ptr<task> > tasks;

  for (unsigned long i (0); i < thread_count; ++i)
  {
    details::shared_ptr<task> t (new (details::shared) task (*db, i));
    tasks.push_back (t);

    threads.push_back (
      details::shared_ptr<details::thread> (
        new (details::shared) details::thread (&task::execute, t.get ())));
  }

  bool r (true);

  for (unsigned long i (0); i < thread_count; ++i)
    if (threads[i]->join () != 0)
      r = false;

  {
    typedef odb::result<object> result;

    transaction t (db->begin ());
    result r (db->query<object> ());

    for (result::iterator i (r.begin ()); i != r.end (); ++i)
      db->erase<object> (i->id_);

    t.commit ();
  }

  return r;
}

int
main (int argc, char* argv[])
{
  try
  {
    if (!(test (argc, argv, 0) &&
          test (argc, argv, thread_count - 1) &&
          test (argc, argv, thread_count / 2) &&
          test (argc, argv, thread_count / 4)))
      return 1;
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
