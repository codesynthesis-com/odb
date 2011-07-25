// file      : odb/pgsql/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::atol
#include <cassert>
#include <sstream> // istringstream

#include <libpq-fe.h>

#include <odb/exceptions.hxx> // object_not_persistent

#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/result-ptr.hxx>
#include <odb/pgsql/error.hxx>

#include <odb/pgsql/details/endian-traits.hxx>

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
        deallocate ();
      }
      catch (...)
      {
      }
    }

    void statement::
    deallocate ()
    {
      if (deallocated_)
        return;

      string s ("deallocate \"");
      s += name_;
      s += "\"";

      result_ptr r (PQexec (conn_.handle (), s.c_str ()));
      deallocated_ = true;
    }

    statement::
    statement (connection& conn,
               const string& name,
               const string& stmt,
               const Oid* types,
               size_t types_count)
        : conn_ (conn),
          name_ (name),
          deallocated_ (false)
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

        n.formats[i] = 1;

        if (current_bind.is_null != 0 && *current_bind.is_null)
        {
          n.values[i] = 0;
          n.lengths[i] = 0;
          continue;
        }

        n.values[i] = reinterpret_cast<char*> (current_bind.buffer);

        size_t l;

        switch (current_bind.type)
        {
        case bind::boolean:
          {
            l = sizeof (bool);
            break;
          }
        case bind::smallint:
          {
            l = sizeof (short);
            break;
          }
        case bind::integer:
          {
            l = sizeof (int);
            break;
          }
        case bind::bigint:
          {
            l = sizeof (long long);
            break;
          }
        case bind::real:
          {
            l = sizeof (float);
            break;
          }
        case bind::double_:
          {
            l = sizeof (double);
            break;
          }
        case bind::date:
          {
            l = sizeof (int);
            break;
          }
        case bind::time:
        case bind::timestamp:
          {
            l = sizeof (long long);
            break;
          }
        case bind::numeric:
        case bind::text:
        case bind::bytea:
        case bind::bit:
        case bind::varbit:
          {
            l = *current_bind.size;
            break;
          }
        case bind::uuid:
          {
            // UUID is a 16-byte sequence.
            //
            l = 16;
            break;
          }
        }

        n.lengths[i] = static_cast<int> (l);
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
      int int_row (static_cast<int> (row));

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
          *b.is_null = PQgetisnull (result, int_row, i) == 1;

          if (*b.is_null)
            continue;
        }

        const char* v (PQgetvalue (result, int_row, i));

        switch (b.type)
        {
        case bind::boolean:
          {
            *static_cast<bool*> (b.buffer) =
              *reinterpret_cast<const bool*> (v);

            break;
          }
        case bind::smallint:
          {
            *static_cast<short*> (b.buffer) =
              *reinterpret_cast<const short*> (v);

            break;
          }
        case bind::integer:
          {
            *static_cast<int*> (b.buffer) = *reinterpret_cast<const int*> (v);

            break;
          }
        case bind::bigint:
          {
            *static_cast<long long*> (b.buffer) =
              *reinterpret_cast<const long long*> (v);

            break;
          }
        case bind::real:
          {
            *static_cast<float*> (b.buffer) =
              *reinterpret_cast<const float*> (v);

            break;
          }
        case bind::double_:
          {
            *static_cast<double*> (b.buffer) =
              *reinterpret_cast<const double*> (v);

            break;
          }
        case bind::date:
          {
            *static_cast<int*> (b.buffer) = *reinterpret_cast<const int*> (v);

            break;
          }
        case bind::time:
        case bind::timestamp:
          {
            *static_cast<long long*> (b.buffer) =
              *reinterpret_cast<const long long*> (v);

            break;
          }
        case bind::numeric:
        case bind::text:
        case bind::bytea:
        case bind::bit:
        case bind::varbit:
          {
            *b.size = static_cast<size_t> (PQgetlength (result, int_row, i));

             if (b.capacity < *b.size)
             {
               if (b.truncated)
                 *b.truncated = true;

               r = false;
               continue;
             }

             memcpy (b.buffer, v, *b.size);

             break;
          }
        case bind::uuid:
          {
            memcpy (b.buffer, v, 16);

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
      if (result_ != 0)
        PQclear (result_);
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
          cond_ (&cond),
          native_cond_ (native_cond),
          data_ (data),
          result_ (0),
          row_count_ (0),
          current_row_ (0)

    {
    }

    select_statement::
    select_statement (connection& conn,
                      const std::string& name,
                      const std::string& stmt,
                      const Oid* types,
                      std::size_t types_count,
                      native_binding& native_cond,
                      binding& data)
        : statement (conn, name, stmt, types, types_count),
          cond_ (0),
          native_cond_ (native_cond),
          data_ (data),
          result_ (0),
          row_count_ (0),
          current_row_ (0)

    {
    }

    void select_statement::
    execute ()
    {
      if (result_ != 0)
      {
        PQclear (result_);
        result_ = 0;
      }

      if (cond_ != 0)
        bind_param (native_cond_, *cond_);

      // Use temporary result_ptr to guarantee exception safety.
      //
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

      row_count_ = static_cast<size_t> (PQntuples (h));
      current_row_ = 0;

      result_ = r.release ();
    }

    void select_statement::
    free_result ()
    {
      if (result_ != 0)
      {
        PQclear (result_);
        result_ = 0;
      }

      row_count_ = 0;
      current_row_ = 0;
    }

    bool select_statement::
    next ()
    {
      if (current_row_ <= row_count_)
        ++current_row_;

      return current_row_ <= row_count_;
    }

    select_statement::result select_statement::
    load ()
    {
      if (current_row_ > row_count_)
        return no_data;

      assert (current_row_ > 0);
      return bind_result (data_.bind, data_.count, result_, current_row_ - 1)
        ? success
        : truncated;
    }

    void select_statement::
    reload ()
    {
      assert (current_row_ > 0);
      assert (current_row_ <= row_count_);

      if (!bind_result (data_.bind,
                        data_.count,
                        result_,
                        current_row_ - 1,
                        true))
        assert (false);
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
          native_data_ (native_data),
          id_cached_ (false)
    {
    }

    bool insert_statement::
    execute ()
    {
      id_cached_ = false;

      bind_param (native_data_, data_);

      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    native_data_.count,
                                    native_data_.values,
                                    native_data_.lengths,
                                    native_data_.formats,
                                    1));
      PGresult* h (r.get ());
      ExecStatusType stat;

      if (!is_good_result (h, &stat))
      {
        if (PGRES_FATAL_ERROR == stat)
        {
          string s (PQresultErrorField (h, PG_DIAG_SQLSTATE));

          if (s == "23505")
            return false;
        }

        translate_error (conn_, h);
      }

      return true;
    }

    unsigned long long insert_statement::
    id ()
    {
      if (id_cached_)
        return id_;

      result_ptr r (PQexecParams (conn_.handle (),
                                  "select lastval ()",
                                  0, 0, 0, 0, 0, 1));
      PGresult* h (r.get ());

      if (!is_good_result (h))
        translate_error (conn_, h);

      id_ = endian_traits::ntoh (*reinterpret_cast<unsigned long long*> (
                                   PQgetvalue (h, 0, 0)));
      id_cached_ = true;

      return id_;
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
