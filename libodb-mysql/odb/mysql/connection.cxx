// file      : odb/mysql/connection.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <new>    // std::bad_alloc
#include <string>

#include <odb/mysql/database.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/transaction.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/error.hxx>
#include <odb/mysql/exceptions.hxx>
#include <odb/mysql/statement-cache.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
    connection::
    connection (connection_factory& cf)
        : odb::connection (cf), failed_ (false), active_ (0)
    {
      if (mysql_init (&mysql_) == 0)
        throw bad_alloc ();

      handle_.reset (&mysql_);

      database_type& db (database ());

      if (*db.charset () != '\0')
        // Can only fail if we pass an unknown option.
        //
        mysql_options (handle_, MYSQL_SET_CHARSET_NAME, db.charset ());

      // Force the CLIENT_FOUND_ROWS flag so that UPDATE returns the
      // number of found rows, not the number of changed rows. This
      // is necessary to distinguish between the object-not-persistent
      // and nothing-changed conditions.
      //
      if (mysql_real_connect (handle_,
                              db.host (),
                              db.user (),
                              db.password (),
                              db.db (),
                              db.port (),
                              db.socket (),
                              db.client_flags () | CLIENT_FOUND_ROWS) == 0)
      {
        // We cannot use translate_error() here since there is no connection
        // yet.
        //
        unsigned int e (mysql_errno (handle_));

        if (e == CR_OUT_OF_MEMORY)
          throw bad_alloc ();

        throw database_exception (
          e, mysql_sqlstate (handle_), mysql_error (handle_));
      }

      // Do this after we have established the connection.
      //
      statement_cache_.reset (new statement_cache_type (*this));
    }

    connection::
    connection (connection_factory& cf, MYSQL* handle)
        : odb::connection (cf),
          failed_ (false),
          handle_ (handle),
          active_ (0),
          statement_cache_ (new statement_cache_type (*this))
    {
    }

    connection::
    ~connection ()
    {
      active_ = 0;

      // Destroy prepared query statements before freeing the connections.
      //
      recycle ();
      clear_prepared_map ();

      if (stmt_handles_.size () > 0)
        free_stmt_handles ();
    }

    transaction_impl* connection::
    begin ()
    {
      return new transaction_impl (connection_ptr (inc_ref (this)));
    }

    unsigned long long connection::
    execute (const char* s, size_t n)
    {
      clear ();

      {
        odb::tracer* t;
        if ((t = transaction_tracer ()) ||
            (t = tracer ()) ||
            (t = database ().tracer ()))
        {
          string str (s, n);
          t->execute (*this, str.c_str ());
        }
      }

      if (mysql_real_query (handle_, s, static_cast<unsigned long> (n)))
        translate_error (*this);

      // Get the affected row count, if any. If the statement has a result
      // set (e.g., SELECT), we first need to call mysql_store_result().
      //
      unsigned long long r (0);

      if (mysql_field_count (handle_) == 0)
        r = static_cast<unsigned long long> (mysql_affected_rows (handle_));
      else
      {
        if (MYSQL_RES* rs = mysql_store_result (handle_))
        {
          r = static_cast<unsigned long long> (mysql_num_rows (rs));
          mysql_free_result (rs);
        }
        else
          translate_error (*this);
      }

      return r;
    }

    bool connection::
    ping ()
    {
      if (failed ())
        return false;

      if (!mysql_ping (handle_))
        return true;

      switch (mysql_errno (handle_))
      {
      case CR_SERVER_LOST:
      case CR_SERVER_GONE_ERROR:
        {
          mark_failed ();
          return false;
        }
      default:
        {
          translate_error (*this);
          return false; // Never reached.
        }
      }
    }

    void connection::
    clear_ ()
    {
      active_->cancel (); // Should clear itself from active_.
    }

    MYSQL_STMT* connection::
    alloc_stmt_handle ()
    {
      MYSQL_STMT* stmt (mysql_stmt_init (handle_));

      if (stmt == 0)
        throw bad_alloc ();

      return stmt;
    }

    void connection::
    free_stmt_handle (auto_handle<MYSQL_STMT>& stmt)
    {
      if (active_ == 0)
        stmt.reset ();
      else
      {
        stmt_handles_.push_back (stmt); // May throw.
        stmt.release ();
      }
    }

    void connection::
    free_stmt_handles ()
    {
      for (stmt_handles::iterator i (stmt_handles_.begin ()),
             e (stmt_handles_.end ()); i != e; ++i)
      {
        mysql_stmt_close (*i);
      }

      stmt_handles_.clear ();
    }

    // connection_factory
    //
    connection_factory::
    ~connection_factory ()
    {
    }

    void connection_factory::
    database (database_type& db)
    {
      odb::connection_factory::db_ = &db;
      db_ = &db;
    }
  }
}
