// file      : odb/pgsql/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::atol
#include <cassert>
#include <sstream> // istringstream

#include <odb/exceptions.hxx> // object_not_persistent

#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/error.hxx>
#include <odb/pgsql/endian-traits.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    using namespace details;

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
    bind_param (native_binding& n, const binding& b)
    {
      assert (n.count == b.count);

      for (size_t i (0); i < n.count; ++i)
      {
        const bind& current_bind (b.bind[i]);

        if (current_bind.is_null != 0 && *current_bind.is_null)
        {
          n.values[i] = 0;
          continue;
        }

        n.values[i] = reinterpret_cast<char*> (current_bind.buffer);

        // Use text format for numeric/decimal types and binary format
        // for all others.
        //
        if (current_bind.type == bind::numeric)
          n.formats[i] = 0;
        else
        {
          n.formats[i] = 1;
          n.lengths[i] = static_cast<int> (*current_bind.size);
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

        const char* s (PQgetvalue (result, static_cast<int> (row), i));

        switch (b.type)
        {
        case bind::smallint:
          {
            *static_cast<short*> (b.buffer) = endian_traits::ntoh (
              *reinterpret_cast<const short*> (s));

            break;
          }
        case bind::integer:
          {
            *static_cast<int*> (b.buffer) = endian_traits::ntoh (
              *reinterpret_cast<const int*> (s));

            break;
          }
        case bind::bigint:
          {
            *static_cast<long long*> (b.buffer) =
              endian_traits::ntoh (*reinterpret_cast<const long long*> (s));

            break;
          }
        case bind::real:
          {
            *static_cast<float*> (b.buffer) = endian_traits::ntoh (
              *reinterpret_cast<const float*> (s));

            break;
          }
        case bind::double_:
          {
            *static_cast<double*> (b.buffer) = endian_traits::ntoh (
              *reinterpret_cast<const double*> (s));

            break;
          }
        case bind::numeric:
        case bind::text:
        case bind::bytea:
        default:
          {
            *b.size = static_cast<size_t> (
              PQgetlength (result, static_cast<int> (row), i));

             if (b.capacity < *b.size)
             {
               if (b.truncated)
                 *b.truncated = true;

               r = false;
               continue;
             }

             memcpy (b.buffer, s, *b.size);

             break;
          }
        }
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
          data_ (data),
          row_count_ (0),
          current_row_ (0)

    {
    }

    void select_statement::
    execute ()
    {
      result_.reset ();
      bind_param (native_cond_, cond_);

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

      row_count_ = static_cast<size_t> (PQntuples (h));
      current_row_ = 0;
    }

    select_statement::result select_statement::
    fetch ()
    {
      if (current_row_ >= row_count_)
        return no_data;

      PGresult* h (result_.get ());

      if (bind_result (data_.bind, data_.count, h, current_row_))
      {
        ++current_row_;
        return success;
      }

      return truncated;
    }

    void select_statement::
    refetch ()
    {
      assert (current_row_ < row_count_);

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
      bind_param (native_data_, data_);

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

      result_ptr r2 (PQexecParams (conn_.handle (),
                                   "select lastval ()",
                                   0, 0, 0, 0, 0, 1));
      PGresult* h2 (r2.get ());

      if (!is_good_result (h2))
        translate_error (conn_, h2);

      id_ = endian_traits::ntoh (*reinterpret_cast<long long*> (
                                   PQgetvalue (h2, 0, 0)));

      return true;
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
      // We assume that the values, lengths, and formats members of
      // native_cond_ are suffixes of the corresponding members of
      // native_data_.
      //
      bind_param (native_data_, data_);
      bind_param (native_cond_, cond_);

      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    native_data_.count + native_cond_.count,
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
      bind_param (native_cond_, cond_);

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
      {
        // @@ Using stringstream conversion for now. See if we can optimize
        // this (atoll possibly, even though it is not standard).
        //
        istringstream ss (s);
        ss >> count;
      }

      return count;
    }
  }
}
