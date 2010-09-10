// file      : common/threads/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
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

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb;

const unsigned long thread_count = 32;
const unsigned long iteration_count = 100;

struct task
{
  task (database& db, unsigned long n)
      : db_ (db), n_ (n)
  {
  }

  void
  execute ()
  {
    try
    {
      for (unsigned long i (0); i < iteration_count; ++i)
      {
        unsigned long id ((n_ * iteration_count + i) * 3);

        {
          object o1 (id, "frist object");
          object o2 (id + 1, "second object");
          object o3 (id + 2, "third object");

          transaction t (db_.begin_transaction ());
          db_.persist (o1);
          db_.persist (o2);
          db_.persist (o3);
          t.commit ();
        }

        {
          transaction t (db_.begin_transaction ());
          auto_ptr<object> o (db_.load<object> (id));
          assert (o->str_ == "frist object");
          o->str_ = "another value";
          db_.store (*o);
          t.commit ();
        }

        {
          typedef odb::query<object> query;
          typedef odb::result<object> result;

          transaction t (db_.begin_transaction ());
          result r (db_.query<object> (query::str == "another value"));

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
          transaction t (db_.begin_transaction ());
          db_.erase<object> (id);
          t.commit ();
        }
      }
    }
    catch (const odb::exception& e)
    {
      cerr << e.what () << endl;
    }
  }

  static void*
  execute (void* arg)
  {
    static_cast<task*> (arg)->execute ();
    return 0;
  }

  database& db_;
  unsigned long n_;
};

void
test (int argc, char* argv[], size_t max_connections)
{
  auto_ptr<database> db (create_database (argc, argv, max_connections));

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

  for (unsigned long i (0); i < thread_count; ++i)
    threads[i]->join ();

  {
    typedef odb::result<object> result;

    transaction t (db->begin_transaction ());
    result r (db->query<object> ());
    r.cache ();

    for (result::iterator i (r.begin ()); i != r.end (); ++i)
      db->erase<object> (i->id_);

    t.commit ();
  }
}

int
main (int argc, char* argv[])
{
  try
  {
    test (argc, argv, 0);
    test (argc, argv, thread_count - 1);
    test (argc, argv, thread_count / 2);
    test (argc, argv, thread_count / 4);
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
