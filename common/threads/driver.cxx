// file      : common/threads/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test operations in a multi-threaded environment.
//

#include <vector>
#include <memory>   // std::auto_ptr
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

using details::shared;
using details::shared_ptr;
using details::thread;

const size_t thread_count = 32;
const size_t iteration_count = 100;

struct task
{
  task (database& db, size_t n)
      : db_ (db), n_ (n)
  {
  }

  void
  execute ()
  {
    try
    {
      for (size_t i (0); i < iteration_count; ++i)
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
  size_t n_;
};


int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    vector<shared_ptr<thread> > threads;
    vector<shared_ptr<task> > tasks;

    for (size_t i (0); i < thread_count; ++i)
    {
      shared_ptr<task> t (new (shared) task (*db, i));
      tasks.push_back (t);

      threads.push_back (
        shared_ptr<thread> (
          new (shared) thread (&task::execute, t.get ())));
    }

    for (size_t i (0); i < thread_count; ++i)
      threads[i]->join ();
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }

  // pthread_exit (0);
}
