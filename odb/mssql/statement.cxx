// file      : odb/mssql/statement.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstring> // std::strlen
#include <cassert>

#include <odb/tracer.hxx>

#include <odb/mssql/mssql.hxx>
#include <odb/mssql/database.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/statement.hxx>
#include <odb/mssql/exceptions.hxx>
#include <odb/mssql/error.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    // Mapping of bind::buffer_type to SQL_* SQL types.
    //
    static const SQLSMALLINT sql_type_lookup [bind::last] =
    {
      SQL_BIT,        // bind::bit
      SQL_TINYINT,    // bind::tinyint
      SQL_SMALLINT,   // bind::smallint
      SQL_INTEGER,    // bind::integer
      SQL_BIGINT,     // bind::bigint

      SQL_REAL,       // bind::float4
      SQL_DOUBLE,     // bind::float8

      SQL_VARCHAR,    // bind::string
      SQL_VARCHAR,    // bind::long_string

      SQL_WVARCHAR,   // bind::nstring
      SQL_WVARCHAR,   // bind::long_nstring

      SQL_VARBINARY,  // bind::binary
      SQL_VARBINARY   // bind::long_binary
    };

    // Mapping of bind::buffer_type to SQL_C_* C types.
    //
    static const SQLSMALLINT c_type_lookup [bind::last] =
    {
      SQL_C_BIT,      // bind::bit
      SQL_C_UTINYINT, // bind::tinyint
      SQL_C_SSHORT,   // bind::smallint
      SQL_C_SLONG,    // bind::integer
      SQL_C_SBIGINT,  // bind::bigint

      SQL_C_FLOAT,    // bind::float4
      SQL_C_DOUBLE,   // bind::float8

      SQL_C_CHAR,     // bind::string
      SQL_C_CHAR,     // bind::long_string

      SQL_C_WCHAR,    // bind::nstring
      SQL_C_WCHAR,    // bind::long_nstring

      SQL_C_BINARY,   // bind::binary
      SQL_C_BINARY    // bind::long_binary
    };

    //
    // statement
    //

    statement::
    statement (connection& conn, const string& text)
        : conn_ (conn), text_copy_ (text), text_ (text_copy_.c_str ())
    {
      init (text_copy_.size ());
    }

    statement::
    statement (connection& conn, const char* text, bool copy)
        : conn_ (conn)
    {
      size_t n;

      if (copy)
      {
        text_copy_ = text;
        text_ = text_copy_.c_str ();
        n = text_copy_.size ();
      }
      else
      {
        text_ = text;
        n = strlen (text_);
      }

      init (n);
    }

    void statement::
    init (size_t text_size)
    {
      SQLRETURN r;

      // Allocate the handle.
      //
      {
        SQLHANDLE h;
        r = SQLAllocHandle (SQL_HANDLE_STMT, conn_.handle (), &h);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_);

        stmt_.reset (h);
      }

      // Disable escape sequences.
      //
      r = SQLSetStmtAttr (stmt_,
                          SQL_ATTR_NOSCAN,
                          (SQLPOINTER) SQL_NOSCAN_OFF,
                          0);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      // Prepare the statement.
      //
      r = SQLPrepare (stmt_, (SQLCHAR*) text_, (SQLINTEGER) text_size);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->prepare (conn_, *this);
      }
    }

    statement::
    ~statement ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->deallocate (conn_, *this);
      }
    }

    const char* statement::
    text () const
    {
      return text_;
    }

    void statement::
    bind_param (bind* b, size_t n)
    {
      SQLRETURN r;

      n++; // Parameters are counted from 1.

      for (size_t i (1); i < n; ++i, ++b)
      {
        SQLULEN col_size;
        SQLPOINTER buf;

        switch (b->type)
        {
        case bind::long_string:
        case bind::long_binary:
          {
            buf = (SQLPOINTER) b;
            col_size = b->capacity != 0
              ? (SQLULEN) b->capacity
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::long_nstring:
          {
            buf = (SQLPOINTER) b;
            col_size = b->capacity != 0
              ? (SQLULEN) b->capacity / 2 // In characters, not bytes.
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::string:
        case bind::binary:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = (SQLULEN) b->capacity;
            break;
          }
        case bind::nstring:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = (SQLULEN) b->capacity / 2; // In characters, not bytes.
            break;
          }
        default:
          {
            buf = (SQLPOINTER) b->buffer;
            break;
          }
        }

        r = SQLBindParameter (
          stmt_,
          (SQLUSMALLINT) i,
          SQL_PARAM_INPUT,
          c_type_lookup[b->type],
          sql_type_lookup[b->type],
          col_size,
          0, // decimal digits
          buf,
          0, // buffer capacity (shouldn't be needed for input parameters)
          b->size_ind);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
      }
    }

    size_t statement::
    bind_result (bind* b, size_t n)
    {
      SQLRETURN r;

      size_t i (0);
      for (bind* end (b + n); b != end; ++b)
      {
        switch (b->type)
        {
        case bind::long_string:
        case bind::long_nstring:
        case bind::long_binary:
          {
            // Long data is not bound.
            //
            continue;
          }
        default:
          {
            break;
          }
        }

        r = SQLBindCol (stmt_,
                        (SQLUSMALLINT) (i + 1), // Results are counted from 1.
                        c_type_lookup[b->type],
                        (SQLPOINTER) b->buffer,
                        b->capacity,
                        b->size_ind);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        ++i;
      }

      return i;
    }

    SQLRETURN statement::
    execute ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      SQLRETURN r (SQLExecute (stmt_));

      if (r == SQL_NEED_DATA)
      {
        details::buffer& tmp_buf (conn_.long_buffer ());

        if (tmp_buf.capacity () == 0)
          tmp_buf.capacity (4096);

        bind* b;
        for (;;)
        {
          r = SQLParamData (stmt_, (SQLPOINTER*) &b);

          // If we get anything other than SQL_NEED_DATA, then this is
          // the return code of SQLExecute().
          //
          if (r != SQL_NEED_DATA)
            break;

          // See if we have a NULL value.
          //
          if (*b->size_ind == SQL_NULL_DATA)
          {
            r = SQLPutData (stmt_, 0, SQL_NULL_DATA);

            if (!SQL_SUCCEEDED (r))
              translate_error (r, conn_, stmt_);
          }
          else
          {
            long_callback& cb (*static_cast<long_callback*> (b->buffer));

            size_t position (0);
            for (;;)
            {
              size_t n;
              const void* buf;
              chunk_type chunk;

              cb.callback.param (
                cb.context.param,
                &position,
                &buf,
                &n,
                &chunk,
                tmp_buf.data (),
                tmp_buf.capacity ());

              r = SQLPutData (
                stmt_,
                (SQLPOINTER) buf,
                chunk != null_chunk ? (SQLLEN) n : SQL_NULL_DATA);

              if (!SQL_SUCCEEDED (r))
                translate_error (r, conn_, stmt_);

              if (chunk == one_chunk ||
                  chunk == last_chunk ||
                  chunk == null_chunk)
                break;
            }
          }
        }
      }

      return r;
    }

    void statement::
    stream_result (bind* b, size_t i, size_t n)
    {
      details::buffer& tmp_buf (conn_.long_buffer ());

      if (tmp_buf.capacity () == 0)
        tmp_buf.capacity (4096);

      SQLRETURN r;

      for (bind* end (b + n); b != end; ++b)
      {
        bool char_data;

        switch (b->type)
        {
        case bind::long_string:
        case bind::long_nstring:
          {
            char_data = true;
            break;
          }
        case bind::long_binary:
          {
            char_data = false;
            break;
          }
        default:
          {
            continue; // Not long data.
          }
        }

        long_callback& cb (*static_cast<long_callback*> (b->buffer));

        // First determine if the value is NULL as well as try to
        // get the total data size.
        //
        SQLLEN si;
        r = SQLGetData (stmt_,
                        (SQLUSMALLINT) (i + 1),
                        c_type_lookup[b->type],
                        tmp_buf.data (), // Dummy value.
                        0,
                        &si);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        void* buf;
        size_t size (0);
        size_t position (0);
        size_t size_left (si == SQL_NO_TOTAL ? 0 : static_cast<size_t> (si));

        chunk_type c (si == SQL_NULL_DATA
                      ? null_chunk
                      : (si == 0 ? one_chunk : first_chunk));

        for (;;)
        {
          cb.callback.result (
                cb.context.result,
                &position,
                &buf,
                &size,
                c,
                size_left,
                tmp_buf.data (),
                tmp_buf.capacity ());

          if (c == last_chunk || c == one_chunk || c == null_chunk)
            break;

          // SQLGetData() can keep returning SQL_SUCCESS_WITH_INFO (truncated)
          // with SQL_NO_TOTAL for all the calls except the last one. For the
          // last call we should get SQL_SUCCESS and the size_indicator should
          // contain a valid value.
          //
          r = SQLGetData (stmt_,
                          (SQLUSMALLINT) (i + 1),
                          c_type_lookup[b->type],
                          (SQLPOINTER) buf,
                          (SQLLEN) size,
                          &si);

          if (r == SQL_SUCCESS)
          {
            assert (si != SQL_NO_TOTAL);

            // Actual amount of data copied to the buffer (appears not to
            // include the NULL teminator).
            //
            size = static_cast<size_t> (si);
            c = last_chunk;
          }
          else if (r == SQL_SUCCESS_WITH_INFO)
          {
            if (char_data)
              size--; // NULL terminator.

            c = next_chunk;
          }
          else
            translate_error (r, conn_, stmt_);

          // Update the total.
          //
          if (size_left != 0)
            size_left -= size;
        }

        ++i;
      }
    }

    //
    // select_statement
    //

    select_statement::
    select_statement (connection& conn,
                      const string& t,
                      binding& param,
                      binding& result)
        : statement (conn, t), result_ (result), executed_ (false)
    {
      bind_param (param.bind, param.count);
      first_long_ = bind_result (result.bind, result.count);
    }

    select_statement::
    select_statement (connection& conn,
                      const char* t,
                      binding& param,
                      binding& result,
                      bool ct)
        : statement (conn, t, ct), result_ (result), executed_ (false)
    {
      bind_param (param.bind, param.count);
      first_long_ = bind_result (result.bind, result.count);
    }

    select_statement::
    select_statement (connection& conn, const string& t, binding& result)
        : statement (conn, t), result_ (result), executed_ (false)
    {
      first_long_ = bind_result (result.bind, result.count);
    }

    select_statement::
    select_statement (connection& conn,
                      const char* t,
                      binding& result,
                      bool ct)
        : statement (conn, t, ct), result_ (result), executed_ (false)
    {
      first_long_ = bind_result (result.bind, result.count);
    }

    select_statement::
    ~select_statement ()
    {
      if (executed_)
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

    void select_statement::
    execute ()
    {
      if (executed_)
        free_result ();

      SQLRETURN r (statement::execute ());

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      executed_ = true;
    }

    select_statement::result select_statement::
    fetch ()
    {
      SQLRETURN r (SQLFetch (stmt_));

      if (r == SQL_NO_DATA)
        return no_data;

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      return success;
    }

    void select_statement::
    free_result ()
    {
      if (executed_)
      {
        // If we cannot close the cursor, there is no point in trying again.
        //
        executed_ = false;

        SQLRETURN r (SQLCloseCursor (stmt_));

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
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
                      const string& t,
                      binding& param,
                      bool returning)
        : statement (conn, t), returning_ (returning)
    {
      bind_param (param.bind, param.count);

      if (returning)
        init_result ();
    }

    insert_statement::
    insert_statement (connection& conn,
                      const char* t,
                      binding& param,
                      bool returning,
                      bool ct)
        : statement (conn, t, ct), returning_ (returning)
    {
      bind_param (param.bind, param.count);

      if (returning)
        init_result ();
    }

    void insert_statement::
    init_result ()
    {
      SQLRETURN r (
        SQLBindCol (stmt_,
                    1,
                    SQL_C_SBIGINT,
                    (SQLPOINTER) &id_,
                    sizeof (id_),
                    &id_size_ind_));

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);
    }

    bool insert_statement::
    execute ()
    {
      SQLRETURN r (statement::execute ());

      if (!SQL_SUCCEEDED (r))
      {
        // Translate the integrity contraint violation (SQLSTATE 23000)
        // to the flase return value. This code is similar to that found
        // in translate_error().
        //
        char sqlstate[SQL_SQLSTATE_SIZE + 1];
        SQLINTEGER native_code;
        SQLSMALLINT msg_size;

        bool cv (false);

        for (SQLSMALLINT i (1);; ++i)
        {
          SQLRETURN r (SQLGetDiagRec (SQL_HANDLE_STMT,
                                      stmt_,
                                      i,
                                      (SQLCHAR*) sqlstate,
                                      &native_code,
                                      0,
                                      0,
                                      &msg_size));

          if (r == SQL_NO_DATA)
            break;
          else if (SQL_SUCCEEDED (r))
          {
            string s (sqlstate);

            if (s == "23000")      // Integrity contraint violation.
              cv = true;
            else if (s != "01000") // General warning.
            {
              // Some other code.
              //
              cv = false;
              break;
            }
          }
          else
          {
            // SQLGetDiagRec() failure.
            //
            cv = false;
            break;
          }
        }

        if (cv)
          return false;
        else
          translate_error (r, conn_, stmt_);
      }

      // Fetch the row containing the id if this statement is returning.
      //
      if (returning_)
      {
        r = SQLFetch (stmt_);

        if (r != SQL_NO_DATA && !SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        {
          SQLRETURN r (SQLCloseCursor (stmt_)); // Don't overwrite r.

          if (!SQL_SUCCEEDED (r))
            translate_error (r, conn_, stmt_);
        }

        if (r == SQL_NO_DATA)
          throw database_exception (
            0,
            "?????",
            "result set expected from a statement with the OUTPUT clause");
      }

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
    update_statement (connection& conn, const string& t, binding& param)
        : statement (conn, t)
    {
      bind_param (param.bind, param.count);
    }

    update_statement::
    update_statement (connection& conn,
                      const char* t,
                      binding& param,
                      bool ct)
        : statement (conn, t, ct)
    {
      bind_param (param.bind, param.count);
    }

    unsigned long long update_statement::
    execute ()
    {
      SQLRETURN r (statement::execute ());

      // SQL_NO_DATA indicates that the statement hasn't affected any rows.
      //
      if (r == SQL_NO_DATA)
        return 0;

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      // Get the number of affected rows.
      //
      SQLLEN rows;
      r = SQLRowCount (stmt_, &rows);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      return static_cast<unsigned long long> (rows);
    }

    //
    // delete_statement
    //

    delete_statement::
    ~delete_statement ()
    {
    }

    delete_statement::
    delete_statement (connection& conn, const string& t, binding& param)
        : statement (conn, t)
    {
      bind_param (param.bind, param.count);
    }

    delete_statement::
    delete_statement (connection& conn,
                      const char* t,
                      binding& param,
                      bool ct)
        : statement (conn, t, ct)
    {
      bind_param (param.bind, param.count);
    }

    unsigned long long delete_statement::
    execute ()
    {
      SQLRETURN r (statement::execute ());

      // SQL_NO_DATA indicates that the statement hasn't affected any rows.
      //
      if (r == SQL_NO_DATA)
        return 0;

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      // Get the number of affected rows.
      //
      SQLLEN rows;
      r = SQLRowCount (stmt_, &rows);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      return static_cast<unsigned long long> (rows);
    }
  }
}
