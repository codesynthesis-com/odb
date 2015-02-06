// file      : common/transaction/callback/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test transaction callbacks.
//

#include <cstddef> // std::size_t
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

using namespace std;
using namespace odb::core;

struct callback
{
  callback (unsigned short v): v_ (v), t_ (0) {}
  callback (unsigned short v, transaction& t): v_ (v), t_ (0) {register_ (t);}
  ~callback () {if (t_ != 0) unregister ();}

  void
  register_ (transaction& t)
  {
    t_ = &t;
    t.callback_register (&func, this, transaction::event_all, v_, &t_);
  }

  void
  unregister ()
  {
    cout << "  unregister callback " << v_ << endl;
    t_->callback_unregister (this);
    t_ = 0;
  }

  void
  update (unsigned short v)
  {
    v_ = v;
    t_->callback_update (this, transaction::event_all, v_, &t_);
  }

private:
  static void
  func (unsigned short event, void* key, unsigned long long data)
  {
    callback& c (*static_cast<callback*> (key));

    const char* en;
    switch (event)
    {
    case transaction::event_commit:
      en = "commit";
      break;
    case transaction::event_rollback:
      en = "rollback";
      break;
    default:
      en = "unknown";
    }

    cout << "  callback " << c.v_ << " " << en << endl;

    assert (data == c.v_);
    assert (c.t_ == 0);
  }

  unsigned short v_;
  transaction* t_;
};

struct failed {};

static void
throw_func (unsigned short, void*, unsigned long long)
{
  throw failed ();
}

static void
dummy_func (unsigned short, void* key, unsigned long long data)
{
  assert (reinterpret_cast<unsigned long long> (key) == data);
}

static void
fill (transaction& t)
{
  // 20 is from odb/transaction.hxx.
  //
  for (size_t i (0); i < 20; ++i)
    t.callback_register (&dummy_func,
                         reinterpret_cast<void*> (i),
                         transaction::event_all,
                         i);
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv, false));

    // We want to test both stack and dynamic slots.
    //
    for (unsigned short i (1); i < 3; ++i)
    {
      // Test basic logic.
      //
      cout << "test " << i << "/001" << endl;

      // Commit callback.
      //
      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        t.commit ();
      }

      // Rollback callback.
      //
      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        t.rollback ();
      }

      // Rollback via exception callback.
      //
      {
        callback c1 (1);

        try
        {
          transaction t (db->begin ());
          if (i == 2) fill (t);
          c1.register_ (t);
          throw failed ();
        }
        catch (const failed&)
        {
        }
      }

      // Unregister callback at the end.
      //
      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        c1.unregister ();
        t.callback_unregister (&c1); // Test unregistering non-registered key.
        t.commit ();
      }

      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        c1.unregister ();
        callback c2 (2, t);
        t.commit ();
      }

      // Unregister callback in the middle.
      //
      cout << "test " << i << "/002" << endl;
      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        callback c2 (2, t);
        callback c3 (3, t);
        c2.unregister ();
        t.commit ();
      }

      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        callback c2 (2, t);
        callback c3 (3, t);
        c2.unregister ();
        callback c4 (4, t); // Using the free slot.
        t.commit ();
      }

      // Test a callback in the middle that throws.
      //
      cout << "test " << i << "/003" << endl;
      try
      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c1 (1, t);
        t.callback_register (&throw_func, 0);
        callback c2 (2, t);
        t.commit ();
      }
      catch (const failed&)
      {
      }

      // Test callback_update().
      //
      {
        transaction t (db->begin ());
        if (i == 2) fill (t);
        callback c (1, t);
        c.update (2);
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
