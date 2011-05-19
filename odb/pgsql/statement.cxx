// file      : odb/pgsql/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::atol
#include <cassert>

#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/result-ptr.hxx>
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
        release ();
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
               size_t n)
        : conn_ (conn),
          name_ (name)
    {
      result_ptr r (PQprepare (conn_.handle (),
                               name_.c_str (),
                               stmt.c_str (),
                               n,
                               types));

      if (!is_good_result (r.get ()))
        translate_error (conn_, r.get ());
    }

    void statement::
    release ()
    {
      if (!name_.empty ())
      {
        string s ("deallocate ");
        s += name_;

        transaction t (conn_.database ().begin ());
        conn_.database ().execute (s.c_str ());
        t.commit ();
      }
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
                      size_t n,
                      native_binding& data)
        : statement (conn, name, stmt, types, n),
          data_ (data)
    {
    }

    bool insert_statement::
    execute ()
    {
      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    data_.count,
                                    data_.values,
                                    data_.lengths,
                                    data_.formats,
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
                      size_t n,
                      native_binding& cond,
                      native_binding& data)
        : statement (conn, name, stmt, types, n),
          cond_ (cond),
          data_ (data)
    {
    }

    void update_statement::
    execute ()
    {
      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    cond_.count,
                                    cond_.values,
                                    cond_.lengths,
                                    cond_.formats,
                                    1));

      PGresult* h (r.get ());

      if (!is_good_result (h))
        translate_error (conn_, h);

      const char* s (PQcmdTuples (h));

      if (s[0] == '0')
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
                      size_t n,
                      native_binding& cond)
        : statement (conn, name, stmt, types, n),
          cond_ (cond)
    {
    }

    unsigned long long delete_statement::
    execute ()
    {
      result_ptr r (PQexecPrepared (conn_.handle (),
                                    name_.c_str (),
                                    cond_.count,
                                    cond_.values,
                                    cond_.lengths,
                                    cond_.formats,
                                    1));
      PGresult* h (r.get ());

      if (!is_good_result (h))
        translate_error (conn_, h);

      const char* s = PQcmdTuples (h);
      unsigned long long count;

      if (s[0] != '\0' && s[1] == '\0')
        count = static_cast<unsigned long long> (s[0] - '0');
      else
        count = static_cast<unsigned long long> (atol (s));

      return count;
    }
  }
}
