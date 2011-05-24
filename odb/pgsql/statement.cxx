// file      : odb/pgsql/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::atol
#include <cassert>

#include <odb/exceptions.hxx> // object_not_persistent

#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/error.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    //
    // statement
    //

    statement::
    ~statement ()
    {
      try
      {
        string s ("deallocate ");
        s += name_;
        PQexec (conn_.handle (), s.c_str ());
      }
      catch (...)
      {
      }
    }

    statement::
    statement (connection& conn,
               const string& name,
               const string& stmt,
               const Oid* types,
               size_t types_count)
        : conn_ (conn),
          name_ (name)
    {
      result_ptr r (PQprepare (conn_.handle (),
                               name_.c_str (),
                               stmt.c_str (),
                               static_cast<int> (types_count),
                               types));

      if (!is_good_result (r.get ()))
        translate_error (conn_, r.get ());
    }

    void statement::
    bind_param (native_binding& n, const bind* p, size_t c)
    {
      assert (n.count == c);

      for (size_t i (0); i < c; ++i)
      {
        const bind& b (p[i]);

        if (b.is_null != 0 && *b.is_null)
        {
          n.values[i] = 0;
          continue;
        }

        n.values[i] = reinterpret_cast<char*> (b.buffer);

        // Use text format for numeric/decimal types and binary format
        // for all others.
        //
        if (b.type == bind::numeric)
          n.formats[i] = 0;
        else
        {
          n.formats[i] = 1;
          n.lengths[i] = static_cast<int> (*b.size);
        }
      }
    }

    bool statement::
    bind_result (bind* p,
                 size_t count,
                 PGresult* result,
                 size_t row,
                 bool truncated)
    {
      bool r (true);

      assert (static_cast<size_t> (PQnfields (result)) == count);

      for (int i (0); i < static_cast<int> (count); ++i)
      {
        const bind& b (p[i]);

        if (truncated && (b.truncated == 0 || !*b.truncated))
          continue;

        if (b.truncated != 0)
          *b.truncated = false;

        // Check for NULL unless we are reloading a truncated result.
        //
        if (!truncated)
        {
          *b.is_null = PQgetisnull (result, static_cast<int> (row), i) == 1;

          if (*b.is_null)
            continue;
        }

        // @@ Revise this.
        //
        size_t buffer_len;

        switch (b.type)
        {
        case bind::smallint:
          {
            buffer_len = 2;
            break;
          }
        case bind::integer:
          {
            buffer_len = 4;
            break;
          }
        case bind::bigint:
          {
            buffer_len = 8;
            break;
          }
        case bind::real:
          {
            buffer_len = 4;
            break;
          }
        case bind::dbl:
          {
            buffer_len = 8;
            break;
          }
        case bind::numeric:
        case bind::text:
        case bind::bytea:
        default:
          {
            buffer_len = static_cast<size_t> (
              PQgetlength (result, static_cast<int> (row), i));

            *b.size = buffer_len;

             if (b.capacity < *b.size)
             {
               if (b.truncated)
                 *b.truncated = true;

               r = false;
               continue;
             }

             break;
          }
        };

        memcpy (b.buffer,
                PQgetvalue (result, static_cast<int> (row), i),
                buffer_len);
      }

      return r;
    }

    //
    // select_statement
    //

    select_statement::
    ~select_statement ()
    {
    }

    select_statement::
    select_statement (connection& conn,
                      const std::string& name,
                      const std::string& stmt,
                      const Oid* types,
                      std::size_t types_count,
                      binding& cond,
                      native_binding& native_cond,
                      binding& data)
        : statement (conn, name, stmt, types, types_count),
          cond_ (cond),
          native_cond_ (native_cond),
          data_ (data)
    {
    }

    void select_statement::
    execute ()
    {
      result_.reset ();

      if (cond_.version != native_cond_.version)
      {
        bind_param (native_cond_, cond_.bind, cond_.count);

        native_cond_.version = cond_.version;
      }
      else
      {
        for (size_t i (0); i < cond_.count; ++i)
          native_cond_.lengths[i] = static_cast<int> (*cond_.bind[i].size);
      }

      result_.reset (PQexecPrepared (conn_.handle (),
                                     name_.c_str (),
                                     native_cond_.count,
                                     native_cond_.values,
                                     native_cond_.lengths,
                                     native_cond_.formats,
                                     1));

      PGresult* h (result_.get ());

      if (!is_good_result (h))
        translate_error (conn_, h);

      // Clear the result if it is empty so that fetch () and refetch ()
      // behave correctly.
      //
      else if (PQntuples (h) <= 0)
        result_.reset ();

      current_row_ = 0;
    }

    select_statement::result select_statement::
    fetch ()
    {
      PGresult* h (result_.get ());

      if (h == 0)
        return no_data;

      if (bind_result (data_.bind, data_.count, h, current_row_))
      {
        if (PQntuples (h) <= static_cast<int> (++current_row_))
          result_.reset ();

        return success;
      }

      return truncated;
    }

    void select_statement::
    refetch ()
    {
      assert (result_.get () != 0);

      if (!bind_result (data_.bind,
                        data_.count,
                        result_.get (),
                        current_row_,
                        true))
        assert (false);
    }

    void select_statement::
    free_result ()
    {
      result_.reset ();
    }

    //
    // insert_statement
    //

    insert_statement::
    ~insert_statement ()
    {
    }

    insert_statement::
    insert_statement (connection& conn,
                      const string& name,
                      const string& stmt,
                      const Oid* types,
                      size_t types_count,
                      binding& data,
                      native_binding& native_data)
        : statement (conn, name, stmt, types, types_count),
          data_ (data),
          native_data_ (native_data)
    {
    }

    bool insert_statement::
    execute ()
    {
      if (data_.version != native_data_.version)
      {
        bind_param (native_data_, data_.bind, data_.count);

        native_data_.version = data_.version;
      }
      else
      {
        for (size_t i (0); i < data_.count; ++i)
          native_data_.lengths[i] = static_cast<int> (*data_.bind[i].size);
      }

      result_ptr r1 (PQexecPrepared (conn_.handle (),
                                     name_.c_str (),
                                     native_data_.count,
                                     native_data_.values,
                                     native_data_.lengths,
                                     native_data_.formats,
                                     1));
      PGresult* h1 (r1.get ());
      ExecStatusType stat;

      if (!is_good_result (h1, &stat))
      {
        if (PGRES_FATAL_ERROR == stat)
        {
          string s (PQresultErrorField (h1, PG_DIAG_SQLSTATE));

          if (s == "23505")
            return false;
        }

        translate_error (conn_, h1);
      }

      oid_ = PQoidValue (h);

      return true;
    }

    unsigned long long insert_statement::
    id ()
    {
      // @@ Need to check what InvalidOid evaluates to.
      // Is this function signature required?
      //
      return static_cast<unsigned long long> (oid_);
    }

    //
    // update_statement
    //

    update_statement::
    ~update_statement ()
    {
    }

    update_statement::
    update_statement (connection& conn,
                      const string& name,
                      const string& stmt,
                      const Oid* types,
                      size_t types_count,
                      binding& cond,
                      native_binding& native_cond,
                      binding& data,
                      native_binding& native_data)
        : statement (conn, name, stmt, types, types_count),
          cond_ (cond),
          native_cond_ (native_cond),
          data_ (data),
          native_data_ (native_data)
    {
    }

    void update_statement::
    execute ()
    {
      if (cond_.version != native_cond_.version ||
          data_.version != native_data_.version)
      {
        // We assume that cond_.bind is a suffix of data_.bind.
        //
        bind_param (native_data_, data_.bind, data_.count);

        native_cond_.version = cond_.version;
        native_data_.version = data_.version;
      }
      else
      {
        // We assume that cond_.bind is a suffix of data_.bind.
        //
        for (size_t i (0); i < data_.count; ++i)
          native_data_.lengths[i] = static_cast<int> (*data_.bind[i].size);
      }

      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    native_data_.count,
                                    native_data_.values,
                                    native_data_.lengths,
                                    native_data_.formats,
                                    1));

      PGresult* h (r.get ());

      if (!is_good_result (h))
        translate_error (conn_, h);

      // PQcmdTuples returns a string representing the number of matching
      // rows found. "0" indicates no match.
      //
      if (PQcmdTuples (h)[0] == '0')
        throw object_not_persistent ();
    }

    //
    // delete_statement
    //

    delete_statement::
    ~delete_statement ()
    {
    }

    delete_statement::
    delete_statement (connection& conn,
                      const string& name,
                      const string& stmt,
                      const Oid* types,
                      size_t types_count,
                      binding& cond,
                      native_binding& native_cond)
        : statement (conn, name, stmt, types, types_count),
          cond_ (cond),
          native_cond_ (native_cond)
    {
    }

    unsigned long long delete_statement::
    execute ()
    {
      if (cond_.version != native_cond_.version)
      {
        bind_param (native_cond_, cond_.bind, cond_.count);

        native_cond_.version = cond_.version;
      }
      else
      {
        for (size_t i (0); i < cond_.count; ++i)
          native_cond_.lengths[i] = static_cast<int> (*cond_.bind[i].size);
      }

      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    native_cond_.count,
                                    native_cond_.values,
                                    native_cond_.lengths,
                                    native_cond_.formats,
                                    1));
      PGresult* h (r.get ());

      if (!is_good_result (h))
        translate_error (conn_, h);

      const char* s (PQcmdTuples (h));
      unsigned long long count;

      if (s[0] != '\0' && s[1] == '\0')
        count = static_cast<unsigned long long> (s[0] - '0');
      else
        count = static_cast<unsigned long long> (atol (s));

      return count;
    }
  }
}
