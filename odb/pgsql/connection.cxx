// file      : odb/pgsql/connection.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <new>    // std::bad_alloc
#include <string>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/error.hxx>
#include <odb/pgsql/exceptions.hxx>
#include <odb/pgsql/statement-cache.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    connection::
    connection (database_type& db)
        : db_ (db),
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
    }

    connection::
    ~connection ()
    {
      PQfinish (handle_);
    }
  }
}
