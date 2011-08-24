// file      : odb/pgsql/connection.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
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
#include <odb/pgsql/result-ptr.hxx>

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
        : odb::connection (db),
          db_ (db),
          handle_ (0),
          statement_cache_ (new statement_cache_type (*this))
    {
      handle_ = PQconnectdb (db.conninfo ().c_str ());

      if (handle_ == 0)
        throw bad_alloc ();
      else if (PQstatus (handle_) == CONNECTION_BAD)
      {
        std::string m (PQerrorMessage (handle_));
        PQfinish (handle_);

        throw database_exception (m);
      }

      // Suppress server notifications to stdout.
      //
      PQsetNoticeProcessor (handle_, &odb_pgsql_process_notice, 0);

      // Establish whether date/time values are represented as
      // 8-byte integers.
      //
      if (strcmp (PQparameterStatus (handle_, "integer_datetimes"), "on") != 0)
        throw database_exception ("unsupported binary format for PostgreSQL "
                                  "date-time SQL types");
    }

    connection::
    ~connection ()
    {
      // Deallocate prepared statements before we close the connection.
      //
      statement_cache_.reset ();

      PQfinish (handle_);
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

      result_ptr r (PQexec (handle_, str.c_str ()));
      PGresult* h (r.get ());

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
