// file      : common/prepared/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test prepared query functionality.
//

#include <memory>   // std::auto_ptr, std::unique_ptr
#include <utility>  // std::move
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>
#include <common/config.hxx> // HAVE_CXX11

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

struct params
{
  unsigned short age;
  std::string name;
};

static void
query_factory (const char* name, connection& c)
{
  typedef odb::query<person> query;

  auto_ptr<params> p (new params);
  prepared_query<person> pq (
    c.prepare_query<person> (
      name,
      query::age > query::_ref (p->age) &&
      query::name != query::_ref (p->name)));
  c.cache_query (pq, p);
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    {
      person p1 ("John First",  91);
      person p2 ("John Second", 81);
      person p3 ("John Third",  71);
      person p4 ("John Fourth", 61);
      person p5 ("John Fifth",  51);

      transaction t (db->begin ());
      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      db->persist (p4);
      db->persist (p5);
      t.commit ();
    }

    typedef odb::query<person> query;
    typedef odb::prepared_query<person> prep_query;
    typedef odb::result<person> result;

    // Uncached query in the same transaction.
    //
    {
      transaction t (db->begin ());

      unsigned short age (90);
      prep_query pq (
        db->prepare_query<person> (
          "person-age-query",
          query::age > query::_ref (age)));

      for (unsigned short i (1); i < 6; ++i, age -= 10)
      {
        result r (pq.execute ());
        assert (size (r) == i);
      }

      age = 90;
      result r (pq.execute ());
      result::iterator i (r.begin ());
      assert (i != r.end () && i->name_ == "John First" && i->age_ == 91);
      assert (++i == r.end ());

      t.commit ();
    }

    // Uncached query in multiple transaction.
    //
    {
      connection_ptr c (db->connection ());

      unsigned short age (90);
      prep_query pq (
        c->prepare_query<person> (
          "person-age-query",
          query::age > query::_ref (age)));

      for (unsigned short i (1); i < 6; ++i, age -= 10)
      {
        transaction t (c->begin ());

        result r (pq.execute ());
        assert (size (r) == i);

        t.commit ();
      }

      transaction t (c->begin ());

      age = 90;
      result r (pq.execute ());
      result::iterator i (r.begin ());
      assert (i != r.end () && i->name_ == "John First" && i->age_ == 91);
      assert (++i == r.end ());

      t.commit ();
    }

    // Cached query without parameters.
    //
    {
      for (unsigned short i (1); i < 6; ++i)
      {
        transaction t (db->begin ());

        prep_query pq (db->lookup_query<person> ("person-val-age-query"));

        if (!pq)
        {
          assert (i == 1);
          pq = db->prepare_query<person> (
            "person-val-age-query",
            query::age > 90);
          db->cache_query (pq);
        }
        else if (i == 2)
        {
          try
          {
            db->cache_query (pq);
            assert (false);
          }
          catch (const odb::prepared_already_cached&)
          {
          }
        }

        result r (pq.execute ());
        assert (size (r) == 1);

        t.commit ();
      }
    }

    // Cached query with parameters.
    //
    {
      for (unsigned short i (1); i < 6; ++i)
      {
        transaction t (db->begin ());

        unsigned short* age;
        prep_query pq (db->lookup_query<person> ("person-ref-age-query", age));

        if (!pq)
        {
          assert (i == 1);

#ifdef HAVE_CXX11
          unique_ptr<unsigned short> p (new unsigned short);
#else
          auto_ptr<unsigned short> p (new unsigned short);
#endif
          age = p.get ();
          pq = db->prepare_query<person> (
            "person-ref-age-query",
            query::age > query::_ref (*age));

#ifdef HAVE_CXX11
          db->cache_query (pq, move (p));
#else
          db->cache_query (pq, p);
#endif
        }
        else if (i == 2)
        {
          // Object type mismatch.
          //
          try
          {
            db->lookup_query<int> ("person-ref-age-query", age);
            assert (false);
          }
          catch (const odb::prepared_type_mismatch&)
          {
          }

          // Parameters type mismatch.
          //
          try
          {
            int* age;
            db->lookup_query<person> ("person-ref-age-query", age);
            assert (false);
          }
          catch (const odb::prepared_type_mismatch&)
          {
          }
        }

        *age = 100 - i * 10;
        result r (pq.execute ());
        assert (size (r) == i);

        t.commit ();
      }
    }

    // Cached query with factory.
    //
    {
      db->query_factory ("person-params-query", &query_factory);

      for (unsigned int i (1); i < 6; ++i)
      {
        transaction t (db->begin ());

        params* p;
        prep_query pq (db->lookup_query<person> ("person-params-query", p));
        assert (pq);

        p->age = 100 - i * 10;
        p->name = "John First";
        result r (pq.execute ());
        assert (size (r) == i - 1);

        t.commit ();
      }

      db->query_factory ("person-params-query",
                         database::query_factory_ptr ());
    }

    // Cached query with wildcard factory.
    //
    {
      db->query_factory ("", &query_factory);

      for (unsigned int i (1); i < 6; ++i)
      {
        transaction t (db->begin ());

        params* p;
        prep_query pq (db->lookup_query<person> ("person-params-query-1", p));
        assert (pq);

        p->age = 100 - i * 10;
        p->name = "John First";
        result r (pq.execute ());
        assert (size (r) == i - 1);

        t.commit ();
      }

      db->query_factory ("", database::query_factory_ptr ());
    }

    // Cached query with lambda factory.
    //
#ifdef HAVE_CXX11
    {
      db->query_factory (
        "person-params-query-2",
        [] (const char* name, connection& c)
        {
          typedef odb::query<person> query;

          auto_ptr<params> p (new params);
          prepared_query<person> pq (
            c.prepare_query<person> (
              name,
              query::age > query::_ref (p->age) &&
              query::name != query::_ref (p->name)));
          c.cache_query (pq, p);
        });

      for (unsigned int i (1); i < 6; ++i)
      {
        transaction t (db->begin ());

        params* p;
        prep_query pq (db->lookup_query<person> ("person-params-query-2", p));
        assert (pq);

        p->age = 100 - i * 10;
        p->name = "John First";
        result r (pq.execute ());
        assert (size (r) == i - 1);

        t.commit ();
      }

      db->query_factory ("person-params-query-2",
                         database::query_factory_ptr ());
    }

    // Cached query with lambda factory using closure. Forces nonoptimized
    // representation of std::function.
    //
    {
      const std::string person_name ("John First");

      db->query_factory (
        "person-params-query-3",
        [person_name] (const char* name, connection& c)
        {
          typedef odb::query<person> query;

          prepared_query<person> pq (
            c.prepare_query<person> (
              name,
              query::age > 50 && query::name != person_name));
          c.cache_query (pq);
        });

      {
        transaction t (db->begin ());

        prep_query pq (db->lookup_query<person> ("person-params-query-3"));
        assert (pq);

        result r (pq.execute ());
        assert (size (r) == 4);

        t.commit ();
      }

      db->query_factory ("person-params-query-3",
#ifdef HAVE_CXX11_NULLPTR
                         nullptr
#else
                         database::query_factory_ptr ()
#endif
      );
    }
#endif

    // View prepared query.
    //
    {
      typedef odb::query<person_view> query;
      typedef odb::prepared_query<person_view> prep_query;
      typedef odb::result<person_view> result;

      transaction t (db->begin ());

      unsigned short age (90);
      prep_query pq (
        db->prepare_query<person_view> (
          "person-view-age-query",
          query::age > query::_ref (age)));

      for (unsigned short i (1); i < 6; ++i, age -= 10)
      {
        result r (pq.execute ());
        assert (size (r) == i);
      }

      age = 90;
      result r (pq.execute ());
      result::iterator i (r.begin ());
      assert (i != r.end () && i->name == "John First" && i->age == 91);
      assert (++i == r.end ());

      t.commit ();
    }

    // By-ref parameter image growth.
    //
    {
      transaction t (db->begin ());

      string name;
      prep_query pq (
        db->prepare_query<person> (
          "person-name-query",
          query::name != query::_ref (name)));

      {
        name = "John First";
        result r (pq.execute ());
        assert (size (r) == 4);
      }

      {
        name.assign (2048, 'x');
        result r (pq.execute ());
        assert (size (r) == 5);
      }

      t.commit ();
    }

    // Test execute_one() and execute_value().
    //
    {
      transaction t (db->begin ());

      person p ("John Doe", 23);
      db->persist (p);

      prep_query pq1 (
        db->prepare_query<person> ("query-1", query::id == p.id_));
      prep_query pq0 (
        db->prepare_query<person> ("query-0", query::id == p.id_ + 1));

      {
        auto_ptr<person> p (pq1.execute_one ());
        assert (p.get () != 0 && p->name_ == "John Doe");
      }

      {
        auto_ptr<person> p (pq0.execute_one ());
        assert (p.get () == 0);
      }

      {
        person p;
        assert (pq1.execute_one (p) && p.name_ == "John Doe");
      }

      {
        person p ("", 0);
        assert (!pq0.execute_one (p) &&
                p.id_ == 0 && p.name_.empty () && p.age_ == 0);
      }

      {
        person p (pq1.execute_value ());
        assert (p.name_ == "John Doe");
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
