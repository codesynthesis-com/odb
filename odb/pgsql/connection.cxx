// file      : odb/pgsql/connection.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <new>     // std::bad_alloc
#include <string>
#include <cstring> // std::strcmp
#include <cstdlib> // std::atol

#include <libpq-fe.h>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/error.hxx>
#include <odb/pgsql/exceptions.hxx>
#include <odb/pgsql/statement-cache.hxx>

using namespace std;

extern "C" void
odb_pgsql_process_notice (void*, const char*)
{
}

namespace odb
{
  namespace pgsql
  {
    connection::
    connection (database_type& db)
        : odb::connection (db), db_ (db), failed_ (false)
    {
      handle_.reset (PQconnectdb (db.conninfo ().c_str ()));

      if (handle_ == 0)
        throw bad_alloc ();
      else if (PQstatus (handle_) == CONNECTION_BAD)
        throw database_exception (PQerrorMessage (handle_));

      init ();
    }

    connection::
    connection (database_type& db, PGconn* handle)
        : odb::connection (db), db_ (db), handle_ (handle), failed_ (false)
    {
      init ();
    }

    void connection::
    init ()
    {
      // Establish whether date/time values are represented as
      // 8-byte integers.
      //
      if (strcmp (PQparameterStatus (handle_, "integer_datetimes"), "on") != 0)
        throw database_exception ("unsupported binary format for PostgreSQL "
                                  "date-time SQL types");

      // Suppress server notifications to stdout.
      //
      PQsetNoticeProcessor (handle_, &odb_pgsql_process_notice, 0);

      // Create statement cache.
      //
      statement_cache_.reset (new statement_cache_type (*this));
    }

    connection::
    ~connection ()
    {
      // Destroy prepared query statements before freeing the connections.
      //
      recycle ();
      clear_prepared_map ();
    }

    transaction_impl* connection::
    begin ()
    {
      return new transaction_impl (connection_ptr (inc_ref (this)));
    }

    unsigned long long connection::
    execute (const char* s, std::size_t n)
    {
      // The string may not be '\0'-terminated.
      //
      string str (s, n);

      {
        odb::tracer* t;
        if ((t = transaction_tracer ()) ||
            (t = tracer ()) ||
            (t = database ().tracer ()))
          t->execute (*this, str.c_str ());
      }

      auto_handle<PGresult> h (PQexec (handle_, str.c_str ()));

      unsigned long long count (0);

      if (!is_good_result (h))
        translate_error (*this, h);
      else if (PGRES_TUPLES_OK == PQresultStatus (h))
        count = static_cast<unsigned long long> (PQntuples (h));
      else
      {
        const char* s (PQcmdTuples (h));

        if (s[0] != '\0' && s[1] == '\0')
          count = static_cast<unsigned long long> (s[0] - '0');
        else
          count = static_cast<unsigned long long> (atol (s));
      }

      return count;
    }
  }
}
