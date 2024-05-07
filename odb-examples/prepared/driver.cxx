// file      : prepared/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr
#include <cassert>
#include <utility>  // std::move()
#include <iostream>

#include <odb/database.hxx>
#include <odb/connection.hxx>
#include <odb/transaction.hxx>

#include "database.hxx" // create_database

#include "person.hxx"
#include "person-odb.hxx"

using namespace std;
using namespace odb::core;

struct params
{
  unsigned short age;
  string first;
};

static void
query_factory (const char* name, connection& c)
{
  typedef odb::query<person> query;

  unique_ptr<params> p (new params);
  query q (query::age > query::_ref (p->age) &&
           query::first == query::_ref (p->first));
  prepared_query<person> pq (c.prepare_query<person> (name, q));
  c.cache_query (pq, move (p));
}

static void
print_ages (unsigned short age, odb::result<person> r)
{
  cout << "over " << age << ':';

  for (odb::result<person>::iterator i (r.begin ()); i != r.end (); ++i)
    cout << ' ' << i->age ();

  cout << endl;
}

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    // Create a few persistent person objects.
    //
    {
      person p1 ("John", "First",  91);
      person p2 ("John", "Second", 81);
      person p3 ("John", "Third",  71);
      person p4 ("John", "Fourth", 61);
      person p5 ("John", "Fifth",  51);

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

    // Example of an uncached prepared query in the same transaction.
    //
    {
      transaction t (db->begin ());

      unsigned short age (0);
      query q (query::age > query::_ref (age));
      prep_query pq (db->prepare_query<person> ("person-age-query", q));

      for (age = 90; age > 40; age -= 10)
      {
        result r (pq.execute ());
        print_ages (age, r);
      }

      t.commit ();
    }

    // Example of an uncached prepared query in multiple transactions.
    //
    // Note that here we have to first obtain a connection, then prepare
    // the query using this connection, and finally start each transaction
    // that uses the prepared query on this connection.
    //
    {
      connection_ptr conn (db->connection ());

      unsigned short age (0);
      query q (query::age > query::_ref (age));
      prep_query pq (conn->prepare_query<person> ("person-age-query", q));

      for (age = 90; age > 40; age -= 10)
      {
        transaction t (conn->begin ());

        result r (pq.execute ());
        print_ages (age, r);

        t.commit ();
      }
    }

    // Example of a cached prepared query without by-reference parameters.
    //
    for (unsigned short i (0); i < 5; ++i)
    {
      transaction t (db->begin ());

      prep_query pq (db->lookup_query<person> ("person-val-age-query"));

      if (!pq)
      {
        pq = db->prepare_query<person> (
          "person-val-age-query", query::age > 50);
        db->cache_query (pq);
      }

      result r (pq.execute ());
      print_ages (50, r);

      t.commit ();
    }

    // Example of a cached prepared query with by-reference parameters.
    //
    for (unsigned short age (90); age > 40; age -= 10)
    {
      transaction t (db->begin ());

      unsigned short* age_param;
      prep_query pq (
        db->lookup_query<person> ("person-ref-age-query", age_param));

      if (!pq)
      {
        unique_ptr<unsigned short> p (new unsigned short);
        age_param = p.get ();
        query q (query::age > query::_ref (*age_param));
        pq = db->prepare_query<person> ("person-ref-age-query", q);
        db->cache_query (pq, move (p));
      }

      *age_param = age; // Initialize the parameter.
      result r (pq.execute ());
      print_ages (age, r);

      t.commit ();
    }

    // Example of a cached prepared query that uses a query factory.
    //
    db->query_factory ("person-age-name-query", &query_factory);

    for (unsigned short age (90); age > 40; age -= 10)
    {
      transaction t (db->begin ());

      params* p;
      prep_query pq (db->lookup_query<person> ("person-age-name-query", p));
      assert (pq);

      p->age = age;
      p->first = "John";
      result r (pq.execute ());
      print_ages (age, r);

      t.commit ();
    }

    // In C++11 the above call to query_factory() can be re-written to
    // use a lambda function, for example:
    //
    /*
    db->query_factory (
      "person-age-name-query",
      [] (const char* name, connection& c)
      {
        unique_ptr<params> p (new params);
        query q (query::age > query::_ref (p->age) &&
                 query::first == query::_ref (p->first));
        prepared_query<person> pq (c.prepare_query<person> (name, q));
        c.cache_query (pq, std::move (p));
      });
    */

    // Prepared queries can also be used with views, as shown in the
    // following example.
    //
    {
      typedef odb::query<person_count> query;
      typedef odb::prepared_query<person_count> prep_query;

      transaction t (db->begin ());

      unsigned short age (0);
      query q (query::age > query::_ref (age));
      prep_query pq (
        db->prepare_query<person_count> ("person-count-age-query", q));

      // Because an aggregate query result always contains one element,
      // we use execute_value() insetad of execute() as a shortcut:
      //
      for (age = 90; age > 40; age -= 10)
        cout << "over " << age << ": " << pq.execute_value ().count << endl;

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
