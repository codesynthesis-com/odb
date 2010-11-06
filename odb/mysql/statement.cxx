// file      : odb/mysql/statement.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
    // statement
    //

    statement::
    statement (connection& conn)
        : conn_ (conn), stmt_ (conn_.alloc_stmt_handle ())
    {

    }

    statement::
    ~statement ()
    {
      conn_.free_stmt_handle (stmt_);
    }

    void statement::
    cancel ()
    {
    }

    // select_statement
    //

    select_statement::
    ~select_statement ()
    {
      if (cached_ || conn_.active () == this)
      {
        try
        {
          free_result ();
        }
        catch (...)
        {
        }
      }
    }

    select_statement::
    select_statement (connection& conn,
                      const string& s,
                      binding& cond,
                      binding& data)
        : statement (conn),
          end_ (false),
          cached_ (false),
          rows_ (0),
          cond_ (cond),
          cond_version_ (0),
          data_ (data),
          data_version_ (0)
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_prepare (stmt_, s.c_str (), s.size ()) != 0)
        throw database_exception (stmt_);
    }

    void select_statement::
    execute ()
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (cached_)
        free_result ();

      end_ = false;
      rows_ = 0;

      if (mysql_stmt_reset (stmt_))
        throw database_exception (stmt_);

      if (cond_version_ != cond_.version)
      {
        if (mysql_stmt_bind_param (stmt_, cond_.bind))
          throw database_exception (stmt_);

        cond_version_ = cond_.version;
      }

      if (data_version_ != data_.version)
      {
        if (mysql_stmt_bind_result (stmt_, data_.bind))
          throw database_exception (stmt_);

        data_version_ = data_.version;
      }

      if (mysql_stmt_execute (stmt_))
      {
        unsigned int e (mysql_stmt_errno (stmt_));

        if (e == ER_LOCK_DEADLOCK)
          throw deadlock ();
        else
          throw database_exception (stmt_);
      }

      conn_.active (this);
    }

    void select_statement::
    cache ()
    {
      if (!cached_)
      {
        if (!end_)
        {
          if (mysql_stmt_store_result (stmt_))
          {
            throw database_exception (stmt_);
          }
        }

        cached_ = true;
      }
    }

    std::size_t select_statement::
    result_size ()
    {
      if (!cached_)
        throw result_not_cached ();

      // mysql_stmt_num_rows() returns the number of rows that have been
      // fetched by store_result.
      //
      return rows_ + static_cast<std::size_t> (mysql_stmt_num_rows (stmt_));
    }

    select_statement::result select_statement::
    fetch ()
    {
      // If the result was cached the data image can grow between calls
      // to fetch() as a result of other statements execution.
      //
      if (cached_ && data_version_ != data_.version)
      {
        if (mysql_stmt_bind_result (stmt_, data_.bind))
          throw database_exception (stmt_);

        data_version_ = data_.version;
      }

      int r (mysql_stmt_fetch (stmt_));

      switch (r)
      {
      case 0:
        {
          if (!cached_)
            rows_++;

          return success;
        }
      case MYSQL_NO_DATA:
        {
          end_ = true;
          return no_data;
        }
      case MYSQL_DATA_TRUNCATED:
        {
          if (!cached_)
            rows_++;

          return truncated;
        }
      default:
        {
          throw database_exception (stmt_);
        }
      }
    }

    void select_statement::
    refetch ()
    {
      // Re-fetch columns that were truncated.
      //
      for (size_t i (0); i < data_.count; ++i)
      {
        if (*data_.bind[i].error)
        {
          *data_.bind[i].error = 0;

          if (mysql_stmt_fetch_column (
                stmt_, data_.bind + i, static_cast<unsigned int> (i), 0))
            throw database_exception (stmt_);
        }
      }
    }

    void select_statement::
    free_result ()
    {
      end_ = true;
      cached_ = false;
      rows_ = 0;

      if (mysql_stmt_free_result (stmt_))
        throw database_exception (stmt_);

      if (conn_.active () == this)
        conn_.active (0);
    }

    void select_statement::
    cancel ()
    {
      // If we cached the result, don't free it just yet.
      //
      if (!cached_)
        free_result ();
      else
        conn_.active (0);
    }

    // insert_statement
    //

    insert_statement::
    ~insert_statement ()
    {
    }

    insert_statement::
    insert_statement (connection& conn, const string& s, binding& data)
        : statement (conn), data_ (data), data_version_ (0)
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_prepare (stmt_, s.c_str (), s.size ()) != 0)
        throw database_exception (stmt_);
    }

    bool insert_statement::
    execute ()
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_reset (stmt_))
        throw database_exception (stmt_);

      if (data_version_ != data_.version)
      {
        if (mysql_stmt_bind_param (stmt_, data_.bind))
          throw database_exception (stmt_);

        data_version_ = data_.version;
      }

      if (mysql_stmt_execute (stmt_))
      {
        switch (mysql_stmt_errno (stmt_))
        {
        case ER_DUP_ENTRY:
          {
            return false;
          }
        case ER_LOCK_DEADLOCK:
          {
            throw deadlock ();
          }
        default:
          {
            throw database_exception (stmt_);
          }
        }
      }

      return true;
    }

    // update_statement
    //

    update_statement::
    ~update_statement ()
    {
    }

    update_statement::
    update_statement (connection& conn,
                      const string& s,
                      binding& id,
                      binding& image)
        : statement (conn),
          id_ (id),
          id_version_ (0),
          image_ (image),
          image_version_ (0)
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_prepare (stmt_, s.c_str (), s.size ()) != 0)
        throw database_exception (stmt_);
    }

    void update_statement::
    execute ()
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_reset (stmt_))
        throw database_exception (stmt_);

      if (image_version_ != image_.version || id_version_ != id_.version)
      {
        // Here we assume that the last element in image_.bind is the
        // id parameter.
        //
        if (mysql_stmt_bind_param (stmt_, image_.bind))
          throw database_exception (stmt_);

        id_version_ = id_.version;
        image_version_ = image_.version;
      }

      if (mysql_stmt_execute (stmt_))
      {
        unsigned int e (mysql_stmt_errno (stmt_));

        if (e == ER_LOCK_DEADLOCK)
          throw deadlock ();
        else
          throw database_exception (stmt_);
      }

      my_ulonglong r (mysql_stmt_affected_rows (stmt_));

      if (r > 0)
        return;

      if (r == 0)
        throw object_not_persistent ();
      else
        throw database_exception (stmt_);
    }

    // delete_statement
    //

    delete_statement::
    ~delete_statement ()
    {
    }

    delete_statement::
    delete_statement (connection& conn, const string& s, binding& cond)
        : statement (conn), cond_ (cond), cond_version_ (0)
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_prepare (stmt_, s.c_str (), s.size ()) != 0)
        throw database_exception (stmt_);
    }

    unsigned long long delete_statement::
    execute ()
    {
      if (statement* a = conn_.active ())
        a->cancel ();

      if (mysql_stmt_reset (stmt_))
        throw database_exception (stmt_);

      if (cond_version_ != cond_.version)
      {
        if (mysql_stmt_bind_param (stmt_, cond_.bind))
          throw database_exception (stmt_);

        cond_version_ = cond_.version;
      }

      if (mysql_stmt_execute (stmt_))
      {
        unsigned int e (mysql_stmt_errno (stmt_));

        if (e == ER_LOCK_DEADLOCK)
          throw deadlock ();
        else
          throw database_exception (stmt_);
      }

      my_ulonglong r (mysql_stmt_affected_rows (stmt_));

      if (r == static_cast<my_ulonglong> (-1))
        throw database_exception (stmt_);

      return static_cast<unsigned long long> (r);
    }
  }
}
