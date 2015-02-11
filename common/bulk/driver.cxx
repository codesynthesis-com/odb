// file      : common/bulk/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test bulk database operations.
//

#include <memory>   // std::auto_ptr
#include <vector>
#include <cassert>
#include <iostream>
#include <iterator>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <odb/details/meta/remove-pointer.hxx>

#include <common/config.hxx> // HAVE_CXX11, DATABASE_*
#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

// Sun CC with non-standard STL does not have iterator_traits in which
// case we assume iterator is just a pointer.
//
template <typename I,
#ifndef _RWSTD_NO_CLASS_PARTIAL_SPEC
          typename T = typename iterator_traits<I>::value_type
#else
          typename T = typename odb::details::meta::remove_pointer<I>::result
#endif
          >
struct element_traits;

template <typename I, typename T>
struct element_traits
{
  typedef T type;
  typedef T* pointer;
  typedef std::auto_ptr<T> auto_ptr;

  static T& ref (T& x) {return x;}
  static T* ptr (T* p) {return p;}
};

template <typename I, typename T>
struct element_traits<I, T*>
{
  typedef T type;
  typedef T* pointer;
  typedef std::auto_ptr<T> auto_ptr;

  static T& ref (T* p) {return *p;}
  static T* ptr (T* p) {return p;}
};

template <typename I, typename T>
struct element_traits<I, std::auto_ptr<T> >
{
  typedef T type;
  typedef std::auto_ptr<T> pointer;
  typedef std::auto_ptr<T> auto_ptr;

  static T& ref (const auto_ptr& p) {return *p;}
  static T* ptr (const auto_ptr& p) {return p.get ();}
};

#ifdef HAVE_CXX11
template <typename I, typename T>
struct element_traits<I, std::unique_ptr<T>>
{
  typedef T type;
  typedef std::unique_ptr<T> pointer;
  typedef std::unique_ptr<T> auto_ptr;

  static T& ref (const unique_ptr<T>& p) {return *p;}
  static T* ptr (const unique_ptr<T>& p) {return p.get ();}
};
#endif

template <typename I>
void
persist (const auto_ptr<database>& db, I b, I e, bool cont = true)
{
  typedef element_traits<I> traits;
  typedef typename traits::type type;
  typedef typename traits::auto_ptr auto_ptr;

  {
    transaction t (db->begin ());
    db->persist (b, e, cont);
    t.commit ();
  }

  // Verify we can load the objects via their ids.
  //
  {
    transaction t (db->begin ());

    for (I i (b); i != e; ++i)
    {
      type& x (traits::ref (*i));
      auto_ptr p (db->load<type> (x.id));
      assert (p->n == x.n && p->s == x.s);
    }

    t.commit ();
  }
}

template <typename I>
void
try_persist (const auto_ptr<database>& db, I b, I e, bool cont = true)
{
  try
  {
    persist (db, b, e, cont);
    assert (false);
  }
  catch (const multiple_exceptions& e)
  {
    cout << e.what () << endl << endl;
  }
}

template <typename I>
void
update (const auto_ptr<database>& db, I b, I e,
        bool modify = true, bool cont = true)
{
  typedef element_traits<I> traits;
  typedef typename traits::type type;
  typedef typename traits::auto_ptr auto_ptr;

  if (modify)
  {
    for (I i (b); i != e; ++i)
    {
      type& x (traits::ref (*i));
      x.n++;
      x.s[0]++;
    }
  }

  {
    transaction t (db->begin ());
    db->update (b, e, cont);
    t.commit ();
  }

  // Verify changes.
  //
  {
    transaction t (db->begin ());

    for (I i (b); i != e; ++i)
    {
      type& x (traits::ref (*i));
      auto_ptr p (db->load<type> (x.id));
      assert (p->n == x.n && p->s == x.s);
    }

    t.commit ();
  }
}

template <typename I>
void
try_update (const auto_ptr<database>& db, I b, I e, bool cont = true)
{
  try
  {
    update (db, b, e, false, cont);
    assert (false);
  }
  catch (const multiple_exceptions& e)
  {
    cout << e.what () << endl << endl;
  }
}

template <typename I>
void
erase (const auto_ptr<database>& db, I b, I e)
{
  typedef element_traits<I> traits;
  typedef typename traits::type type;

  {
    transaction t (db->begin ());
    db->erase (b, e);
    t.commit ();
  }

  // Verify the objects are gone.
  //
  {
    transaction t (db->begin ());

    for (I i (b); i != e; ++i)
    {
      type& x (traits::ref (*i));
      typename traits::pointer p (db->find<type> (x.id));
      assert (traits::ptr (p) == 0);
    }

    t.commit ();
  }
}

template <typename T, typename I>
void
erase_id (const auto_ptr<database>& db, I b, I e, bool cont = true)
{
  typedef element_traits<T*> traits;
  typedef T type;

  {
    transaction t (db->begin ());
    db->erase<T> (b, e, cont);
    t.commit ();
  }

  // Verify the objects are gone.
  //
  {
    transaction t (db->begin ());

    for (I i (b); i != e; ++i)
      assert (traits::ptr (db->find<type> (*i)) == 0);

    t.commit ();
  }
}

template <typename T, typename A>
void
try_erase (const auto_ptr<database>& db, const A& a, bool cont = true)
{
  try
  {
    erase_id<T> (db, a, a + sizeof (a) / sizeof (a[0]), cont);
    assert (false);
  }
  catch (const multiple_exceptions& e)
  {
    cout << e.what () << endl << endl;
  }
}


template <typename I>
void
test (const auto_ptr<database>& db, I b, I e)
{
  persist (db, b, e);
  update (db, b, e);
  erase (db, b, e);
}

template <typename T>
vector<T>
fill (std::size_t count)
{
  vector<T> r;

  unsigned int n (1);
  std::string s ("a");

  for (size_t i (0); i != count; ++i)
  {
    r.push_back (T (n, s));
    n++;
    s[0] = (s[0] == 'z' ? 'a' : s[0] + 1);
  }

  return r;
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

#if defined(DATABASE_ORACLE) || defined(DATABASE_MSSQL)

    // Test database class API with various forms of containers
    // and elements (test #6 is a copy).
    //
    {
      using namespace test1;

      {
        object a[2];
        a[0] = object (1, "a");
        a[1] = object (2, "b");
        test (db, a, a + sizeof (a) / sizeof (a[0]));
      }

      {
        vector<object> v;
        v.push_back (object (1, "a"));
        v.push_back (object (2, "b"));
        test (db, v.begin (), v.end ());
      }

      {
        object o1 (1, "a");
        object o2 (2, "b");
        object* a[2] = {&o1, &o2};
        test (db, a, a + sizeof (a) / sizeof (a[0]));
      }

      {
        object o1 (1, "a");
        object o2 (2, "b");
        vector<object*> v;
        v.push_back (&o1);
        v.push_back (&o2);
        test (db, v.begin (), v.end ());
      }

      {
        auto_ptr<auto_object> a[2];
        a[0].reset (new auto_object (1, "a"));
        a[1].reset (new auto_object (2, "b"));
        test (db, a, a + sizeof (a) / sizeof (a[0]));
      }

#ifdef HAVE_CXX11
      {
        vector<unique_ptr<unique_object>> v;
        v.push_back (unique_ptr<unique_object> (new unique_object (1, "a")));
        v.push_back (unique_ptr<unique_object> (new unique_object (2, "b")));
        test (db, v.begin (), v.end ());
      }
#endif

      {
        vector<object> v;
        v.push_back (object (1, "a"));
        v.push_back (object (2, "b"));
        persist (db, v.begin (), v.end ());

        unsigned long id[2] = {v[0].id, v[1].id};
        erase_id<object> (db, id, id + sizeof (id) / sizeof (id[0]));
      }

      {
        vector<object> v;
        v.push_back (object (1, "a"));
        v.push_back (object (2, "b"));
        persist (db, v.begin (), v.end ());

        vector<unsigned long> id;
        id.push_back (v[0].id);
        id.push_back (v[1].id);
        erase_id<object> (db, id.begin (), id.end ());
      }
    }

    // Test various batch sizes.
    //
    {
      using namespace test1;

      {
        vector<object> v; // 0
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (1)); // 1
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (2)); // batch - 1
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (3)); // batch
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (4)); // batch + 1
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (5)); // 2 * batch - 1
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (6)); // 2 * batch
        test (db, v.begin (), v.end ());
      }

      {
        vector<object> v (fill<object> (100)); // 100
        test (db, v.begin (), v.end ());
      }
    }

    // Test object with manually assigned id.
    //
    {
      using namespace test2;

      {
        vector<object> v;
        v.push_back (object ("1", 1, "a"));
        v.push_back (object ("2", 2, "b"));
        test (db, v.begin (), v.end ());
      }

#ifdef HAVE_CXX11
      {
        typedef unique_ptr<unique_object> unique_ptr;

        vector<unique_ptr> v;
        v.push_back (unique_ptr (new unique_object ("1", 1, "a")));
        v.push_back (unique_ptr (new unique_object ("2", 2, "b")));
        test (db, v.begin (), v.end ());
      }
#endif

      // Test const objects.
      //

      {
        const object a[1];
        const object* e (a + sizeof (a) / sizeof (a[0]));

        transaction t (db->begin ());
        db->persist (a, e);
        db->erase (a, e);
        t.commit ();
      }

      {
        object o1 ("1", 1, "a");
        object o2 ("2", 2, "b");

        vector<const object*> v;
        v.push_back (&o1);
        v.push_back (&o2);

        transaction t (db->begin ());
        db->persist (v.begin (), v.end ());
        db->erase (v.begin (), v.end ());
        t.commit ();
      }
    }

    // Test failure.
    //
    {
      using namespace test3;

      vector<object> v;
      v.push_back (object (6, 6));
      v.push_back (object (7, 7));
      v.push_back (object (8, 8));
      v.push_back (object (9, 9));
      v.push_back (object (10, 10));
      v.push_back (object (11, 11));

      persist (db, v.begin (), v.end ());

      // persist
      //
      {
        {
          vector<object> v; // 1
          v.push_back (object (6, 6));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (8, 8));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch + 1
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (8, 8));
          v.push_back (object (9, 9));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2 x batch - 1
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (8, 8));
          v.push_back (object (9, 9));
          v.push_back (object (10, 10));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2 x batch
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (8, 8));
          v.push_back (object (9, 9));
          v.push_back (object (10, 10));
          v.push_back (object (11, 11));
          try_persist (db, v.begin (), v.end ());
        }

        // Mixture of success and failure.
        //

        {
          vector<object> v; // 1
          v.push_back (object (0, 0));
          v.push_back (object (6, 6));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 1
          v.push_back (object (6, 6));
          v.push_back (object (0, 0));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (0, 0));
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (6, 6));
          v.push_back (object (0, 0));
          v.push_back (object (7, 7));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (0, 0));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (0, 0));
          v.push_back (object (8, 8));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (8, 8));
          v.push_back (object (0, 0));
          try_persist (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // mixture
          v.push_back (object (0, 0));
          v.push_back (object (6, 6));
          v.push_back (object (1, 1));
          v.push_back (object (7, 7));
          v.push_back (object (2, 2));
          v.push_back (object (8, 8));
          v.push_back (object (3, 3));
          try_persist (db, v.begin (), v.end ());
        }

        // Test stopping after failure.
        //
        {
          vector<object> v; // batch
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (6, 6));
          v.push_back (object (2, 2));
          v.push_back (object (3, 3));
          try_persist (db, v.begin (), v.end (), false);
        }
      }

      // update
      //
      {
        {
          vector<object> v; // 1
          v.push_back (object (0, 0));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (2, 2));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch + 1
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (2, 2));
          v.push_back (object (3, 3));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2 x batch - 1
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (2, 2));
          v.push_back (object (3, 3));
          v.push_back (object (4, 4));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2 x batch
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (2, 2));
          v.push_back (object (3, 3));
          v.push_back (object (4, 4));
          v.push_back (object (5, 5));
          try_update (db, v.begin (), v.end ());
        }

        // Mixture of success and failure.
        //

        {
          vector<object> v; // 1
          v.push_back (object (6, 6));
          v.push_back (object (0, 0));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 1
          v.push_back (object (0, 0));
          v.push_back (object (6, 6));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (6, 6));
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (0, 0));
          v.push_back (object (6, 6));
          v.push_back (object (1, 1));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // 2
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (6, 6));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (6, 6));
          v.push_back (object (2, 2));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // batch
          v.push_back (object (0, 0));
          v.push_back (object (1, 1));
          v.push_back (object (2, 2));
          v.push_back (object (6, 6));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // mixture
          v.push_back (object (0, 0));
          v.push_back (object (6, 6));
          v.push_back (object (2, 2));
          v.push_back (object (7, 7));
          v.push_back (object (3, 3));
          v.push_back (object (8, 8));
          v.push_back (object (4, 4));
          try_update (db, v.begin (), v.end ());
        }

        {
          vector<object> v; // mixture
          v.push_back (object (0, 0));
          v.push_back (object (2, 2));
          v.push_back (object (3, 3));
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (8, 8));
          v.push_back (object (4, 4));
          try_update (db, v.begin (), v.end ());
        }

        // Test stopping after failure.
        //
        {
          vector<object> v; // batch
          v.push_back (object (6, 6));
          v.push_back (object (7, 7));
          v.push_back (object (0, 0));
          v.push_back (object (8, 8));
          v.push_back (object (9, 9));
          try_update (db, v.begin (), v.end (), false);
        }

        // Test a database exception (unique constraint violation)
        //
        try
        {
          v[0].n++;
          v[2].n++;

          update (db, v.begin (), v.begin () + 3, false);
          assert (false);
        }
        catch (const multiple_exceptions& e)
        {
          assert (e.attempted () == 3 && e.failed () == 2);
          assert (e[0] != 0 && e[1] == 0 && e[2] != 0);
        }
      }

      // erase
      //
      {
        {
          unsigned long a[] = {0}; // 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1}; // 2
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 2}; // batch
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 2, 3}; // batch + 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 2, 3, 4}; // 2 x batch - 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 2, 3, 4, 5}; // 2 x batch
          try_erase<object> (db, a);
        }

        // Mixture of success and failure.
        //

        {
          unsigned long a[] = {6, 0}; // 2
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 6}; // 2
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {6, 0, 1}; // batch
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 6, 1}; // batch
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 6}; // batch
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {6, 0, 1, 2}; // batch + 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 6, 1, 2}; // batch + 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 6, 2}; // batch + 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 2, 6}; // batch + 1
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {6, 0, 7, 1, 8, 2, 9, 3}; // mixture
          try_erase<object> (db, a);
        }

        {
          unsigned long a[] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9}; // mixture
          try_erase<object> (db, a);
        }

        // Test stopping after failure.
        //
        {
          unsigned long a[] = {6, 7, 0, 8, 9};
          try_erase<object> (db, a, false);
        }
      }

      erase (db, v.begin (), v.end ());
    }

    // Test a large batch.
    //
    {
      using namespace test4;

      vector<object> v (fill<object> (5000));
      test (db, v.begin (), v.end ());
    }

    // Test object without id.
    //
    {
      using namespace test5;

      vector<object> v;
      v.push_back (object (1, "a"));
      v.push_back (object (2, "b"));

      {
        transaction t (db->begin ());
        db->persist (v.begin (), v.end ());
        t.commit ();
      }

      {
        typedef odb::query<object> query;
        typedef odb::result<object> result;

        transaction t (db->begin ());

        result r (db->query<object> ("ORDER BY" + query::n));
        result::iterator i (r.begin ());

        assert (i != r.end () && i->n == 1 && i->s == "a");
        assert (++i != r.end () && i->n == 2 && i->s == "b");
        assert (++i == r.end ());

        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->erase_query<object> ();
        t.commit ();
      }
    }

    // Test API with persistent class template instantiations (copy of
    // test #1).
    {
      using namespace test6;

      // Make sure we can still call the non-bulk API.
      //
      {
        object o (0, "z");
        transaction t (db->begin ());
        db->persist (o);
        db->update<object> (o);
        db->reload<object> (o);
        db->erase<object> (o);
        t.commit ();
      }


      // The rest is a copy of test #1.
      //
      {
        object a[2];
        a[0] = object (1, "a");
        a[1] = object (2, "b");
        test (db, a, a + sizeof (a) / sizeof (a[0]));
      }

      {
        vector<object> v;
        v.push_back (object (1, "a"));
        v.push_back (object (2, "b"));
        test (db, v.begin (), v.end ());
      }

      {
        object o1 (1, "a");
        object o2 (2, "b");
        object* a[2] = {&o1, &o2};
        test (db, a, a + sizeof (a) / sizeof (a[0]));
      }

      {
        object o1 (1, "a");
        object o2 (2, "b");
        vector<object*> v;
        v.push_back (&o1);
        v.push_back (&o2);
        test (db, v.begin (), v.end ());
      }

      {
        auto_ptr<auto_object> a[2];
        a[0].reset (new auto_object (1, "a"));
        a[1].reset (new auto_object (2, "b"));
        test (db, a, a + sizeof (a) / sizeof (a[0]));
      }

#ifdef HAVE_CXX11
      {
        vector<unique_ptr<unique_object>> v;
        v.push_back (unique_ptr<unique_object> (new unique_object (1, "a")));
        v.push_back (unique_ptr<unique_object> (new unique_object (2, "b")));
        test (db, v.begin (), v.end ());
      }
#endif

      {
        vector<object> v;
        v.push_back (object (1, "a"));
        v.push_back (object (2, "b"));
        persist (db, v.begin (), v.end ());

        unsigned long id[2] = {v[0].id, v[1].id};
        erase_id<object> (db, id, id + sizeof (id) / sizeof (id[0]));
      }

      {
        vector<object> v;
        v.push_back (object (1, "a"));
        v.push_back (object (2, "b"));
        persist (db, v.begin (), v.end ());

        vector<unsigned long> id;
        id.push_back (v[0].id);
        id.push_back (v[1].id);
        erase_id<object> (db, id.begin (), id.end ());
      }
    }

    // Test optimistic concurrency.
    //
    {
      using namespace test7;

      std::vector<object> v (fill<object> (4));

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (v.begin (), v.end ());
        t.commit ();

        assert (v[0].v != 0 &&
                v[1].v != 0 &&
                v[2].v != 0 &&
                v[3].v != 0);
      }

      // update
      //
      {
        std::vector<object> c (v);

        transaction t (db->begin ());
        db->update (v.begin (), v.end ());
        t.commit ();

        assert (v[0].v > c[0].v &&
                v[1].v > c[1].v &&
                v[2].v > c[2].v &&
                v[3].v > c[3].v);
      }

      {
        object o2 (v[1]);
        object o4 (v[3]);

        o2.n++;
        o4.n++;

        transaction t (db->begin ());
        db->update (o2);
        db->update (o4);
        t.commit ();
      }

      try
      {
        transaction t (db->begin ());
        db->update (v.begin (), v.end ());
        assert (false);
      }
      catch (const multiple_exceptions& e)
      {
        cout << e.what () << endl << endl;
      }

      // erase
      //
      try
      {
        transaction t (db->begin ());
        db->erase (v.begin (), v.end ());
        assert (false);
      }
      catch (const multiple_exceptions& e)
      {
        cout << e.what () << endl << endl;
      }

      {
        transaction t (db->begin ());
        db->reload (v[1]);
        db->reload (v[3]);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->erase (v.begin (), v.end ());
        t.commit ();
      }
    }

    // Test SQL Server optimistic concurrency with ROWVERSION.
    //
#ifdef DATABASE_MSSQL
    {
      using namespace test8;

      std::vector<object> v (fill<object> (4));

      v[0].id = 1;
      v[1].id = 2;
      v[2].id = 3;
      v[3].id = 4;


      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (v.begin (), v.end ());
        t.commit ();

        assert (v[0].v != 0 &&
                v[1].v != 0 &&
                v[2].v != 0 &&
                v[3].v != 0);

        //cerr << v[0].v << endl
        //     << v[1].v << endl
        //     << v[2].v << endl
        //     << v[3].v << endl;
      }

      // update
      //

      /*
      {
        std::vector<object> c (v);

        transaction t (db->begin ());
        db->update (v.begin (), v.end ());
        t.commit ();

        assert (v[0].v > c[0].v &&
                v[1].v > c[1].v &&
                v[2].v > c[2].v &&
                v[3].v > c[3].v);
      }
      */

      {
        object o2 (v[1]);
        object o4 (v[3]);

        o2.n++;
        o4.n++;

        transaction t (db->begin ());
        db->update (o2);
        db->update (o4);
        t.commit ();
      }

      /*
      try
      {
        transaction t (db->begin ());
        db->update (v.begin (), v.end ());
        assert (false);
      }
      catch (const multiple_exceptions& e)
      {
        cout << e.what () << endl << endl;
      }
      */

      // erase
      //
      try
      {
        transaction t (db->begin ());
        db->erase (v.begin (), v.end ());
        assert (false);
      }
      catch (const multiple_exceptions& e)
      {
        assert (e.attempted () == 4 && e.failed () == 4);
      }

      {
        transaction t (db->begin ());
        db->reload (v[1]);
        db->reload (v[3]);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        db->erase (v.begin (), v.end ());
        t.commit ();
      }
    }
#endif

#endif
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
