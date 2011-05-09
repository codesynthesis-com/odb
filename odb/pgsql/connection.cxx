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

using namespace std;

namespace odb
{
  namespace pgsql
  {
    connection::
    connection (database_type& db)
        : db_ (db),
          handle_ (0)
          // active_ (0),
          // statement_cache_ (new statement_cache_type (*this))
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
      // if (stmt_handles_.size () > 0)
      //   free_stmt_handles ();

      PQfinish (handle_);
    }

    // void connection::
    // clear_ ()
    // {
    //   active_->cancel (); // Should clear itself from active_.
    // }

    // MYSQL_STMT* connection::
    // alloc_stmt_handle ()
    // {
    //   MYSQL_STMT* stmt (mysql_stmt_init (handle_));

    //   if (stmt == 0)
    //     throw bad_alloc ();

    //   return stmt;
    // }

    // void connection::
    // free_stmt_handle (MYSQL_STMT* stmt)
    // {
    //   if (active_ == 0)
    //     mysql_stmt_close (stmt);
    //   else
    //     stmt_handles_.push_back (stmt);
    // }

    // void connection::
    // free_stmt_handles ()
    // {
    //   for (stmt_handles::iterator i (stmt_handles_.begin ()),
    //          e (stmt_handles_.end ()); i != e; ++i)
    //   {
    //     mysql_stmt_close (*i);
    //   }

    //   stmt_handles_.clear ();
    // }
  }
}
