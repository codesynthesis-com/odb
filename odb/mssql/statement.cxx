// file      : odb/mssql/statement.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::strlen, std::strstr, std::memset, std::memcpy
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
      SQL_BIT,                // bind::bit
      SQL_TINYINT,            // bind::tinyint
      SQL_SMALLINT,           // bind::smallint
      SQL_INTEGER,            // bind::int_
      SQL_BIGINT,             // bind::bigint

      SQL_DECIMAL,            // bind::decimal
      SQL_DECIMAL,            // bind::smallmoney
      SQL_DECIMAL,            // bind::money

      SQL_FLOAT,              // bind::float4
      SQL_FLOAT,              // bind::float8

      SQL_VARCHAR,            // bind::string
      SQL_VARCHAR,            // bind::long_string

      SQL_WVARCHAR,           // bind::nstring
      SQL_WVARCHAR,           // bind::long_nstring

      SQL_VARBINARY,          // bind::binary
      SQL_VARBINARY,          // bind::long_binary

      SQL_TYPE_DATE,          // bind::date
      SQL_SS_TIME2,           // bind::time
      SQL_TYPE_TIMESTAMP,     // bind::datetime
      SQL_SS_TIMESTAMPOFFSET, // bind::datetimeoffset

      SQL_GUID,               // bind::uniqueidentifier
      SQL_BINARY              // bind::rowversion
    };

    // Mapping of bind::buffer_type to SQL_C_* C types.
    //
    static const SQLSMALLINT c_type_lookup [bind::last] =
    {
      SQL_C_BIT,            // bind::bit
      SQL_C_UTINYINT,       // bind::tinyint
      SQL_C_SSHORT,         // bind::smallint
      SQL_C_SLONG,          // bind::int_
      SQL_C_SBIGINT,        // bind::bigint

      SQL_C_NUMERIC,        // bind::decimal
      SQL_C_BINARY,         // bind::smallmoney
      SQL_C_BINARY,         // bind::money

      SQL_C_FLOAT,          // bind::float4
      SQL_C_DOUBLE,         // bind::float8

      SQL_C_CHAR,           // bind::string
      SQL_C_CHAR,           // bind::long_string

      SQL_C_WCHAR,          // bind::nstring
      SQL_C_WCHAR,          // bind::long_nstring

      SQL_C_BINARY,         // bind::binary
      SQL_C_BINARY,         // bind::long_binary

      SQL_C_TYPE_DATE,      // bind::date
      SQL_C_BINARY,         // bind::time
      SQL_C_TYPE_TIMESTAMP, // bind::datetime
      SQL_C_BINARY,         // bind::datetimeoffset

      SQL_C_GUID,           // bind::uniqueidentifier
      SQL_C_BINARY          // bind::rowversion
    };

    // Mapping of bind::buffer_type to fixed buffer capacity values.
    //
    static const SQLLEN capacity_lookup [bind::last] =
    {
      1,                       // bind::bit
      1,                       // bind::tinyint
      2,                       // bind::smallint
      4,                       // bind::int_
      8,                       // bind::bigint

      sizeof (decimal),        // bind::decimal
      4,                       // bind::smallmoney
      8,                       // bind::money

      4,                       // bind::float4
      8,                       // bind::float8

      0,                       // bind::string
      0,                       // bind::long_string

      0,                       // bind::nstring
      0,                       // bind::long_nstring

      0,                       // bind::binary
      0,                       // bind::long_binary

      sizeof (date),           // bind::date
      sizeof (time),           // bind::time
      sizeof (datetime),       // bind::datetime
      sizeof (datetimeoffset), // bind::datetimeoffset

      16,                      // bind::uniqueidentifier
      8                        // bind::rowversion
    };

    //
    // statement
    //

    statement::
    statement (connection_type& conn,
               const string& text,
               statement_kind sk,
               const binding* process,
               bool optimize)
        : conn_ (conn)
    {
      if (process == 0)
      {
        text_copy_ = text;
        text_ = text_copy_.c_str ();
      }
      else
        text_ = text.c_str (); // Temporary, see init().

      init (text.size (), sk, process, optimize);
    }

    statement::
    statement (connection_type& conn,
               const char* text,
               statement_kind sk,
               const binding* process,
               bool optimize,
               bool copy)
        : conn_ (conn)
    {
      size_t n;

      if (process == 0 && copy)
      {
        text_copy_ = text;
        text_ = text_copy_.c_str ();
        n = text_copy_.size ();
      }
      else
      {
        text_ = text;
        n = strlen (text_); // Potentially temporary, see init().
      }

      init (n, sk, process, optimize);
    }

    void statement::
    init (size_t text_size,
          statement_kind sk,
          const binding* proc,
          bool optimize)
    {
      if (proc != 0)
      {
        switch (sk)
        {
        case statement_select:
          process_select (text_,
                          proc->bind, proc->count,
                          optimize,
                          text_copy_);
          break;
        case statement_insert:
          process_insert (text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '?',
                          text_copy_);
          break;
        case statement_update:
          process_update (text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '?',
                          text_copy_);
          break;
        case statement_delete:
          assert (false);
        }

        text_ = text_copy_.c_str ();
        text_size = text_copy_.size ();
      }

      // Empty statement.
      //
      if (*text_ == '\0')
        return;

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

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->prepare (conn_, *this);
      }

      // Prepare the statement.
      //
      r = SQLPrepareA (stmt_, (SQLCHAR*) text_, (SQLINTEGER) text_size);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);
    }

    statement::
    ~statement ()
    {
      if (stmt_ != 0)
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

      SQLUSMALLINT i (0);
      for (bind* end (b + n); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

        i++; // Column index is 1-based.

        SQLULEN col_size (0);
        SQLSMALLINT digits (0);
        SQLPOINTER buf;

        switch (b->type)
        {
        case bind::decimal:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = (SQLULEN) (b->capacity / 100);   // precision
            digits = (SQLSMALLINT) (b->capacity % 100); // scale
            break;
          }
        case bind::smallmoney:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = 10;
            digits = 4;
            break;
          }
        case bind::money:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = 19;
            digits = 4;
            break;
          }
        case bind::float4:
        case bind::float8:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = (SQLULEN) b->capacity; // precision
            break;
          }
        case bind::long_string:
        case bind::long_binary:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = b->capacity != 0
              ? (SQLULEN) b->capacity
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::long_nstring:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = b->capacity != 0
              ? (SQLULEN) b->capacity / 2 // In characters, not bytes.
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::string:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = b->capacity != 0
              ? (SQLULEN) b->capacity - 1 // Sans the null-terminator.
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::binary:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = b->capacity != 0
              ? (SQLULEN) b->capacity
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::nstring:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = b->capacity != 0
              // In characters, not bytes, and sans the null-terminator.
              ? (SQLULEN) (b->capacity / 2 - 1)
              : SQL_SS_LENGTH_UNLIMITED;
            break;
          }
        case bind::date:
          {
            buf = (SQLPOINTER) b->buffer;
            // Native Client 10.0 requires the correct precision.
            //
            col_size = 10;
            break;
          }
        case bind::time:
          {
            buf = (SQLPOINTER) b->buffer;
            digits = (SQLSMALLINT) b->capacity;

            // Native Client 10.0 requires the correct precision.
            //
            if (digits == 0)
              col_size = 8;
            else
              col_size = (SQLULEN) (digits + 9);

            break;
          }
        case bind::datetime:
          {
            buf = (SQLPOINTER) b->buffer;
            digits = (SQLSMALLINT) b->capacity;

            // Native Client 10.0 requires the correct precision.
            //
            if (digits == 0)
              col_size = 19;
            else if (digits == 8)
            {
              // This is a SMALLDATETIME column which only has the minutes
              // precision. Documentation indicates that the correct numeric
              // precision value for this type is 16.
              //
              digits = 0;
              col_size = 16;
            }
            else
              col_size = (SQLULEN) (digits + 20);

            break;
          }
        case bind::datetimeoffset:
          {
            buf = (SQLPOINTER) b->buffer;
            digits = (SQLSMALLINT) b->capacity;

            // Native Client 10.0 requires the correct precision.
            //
            if (digits == 0)
              col_size = 26;
            else
              col_size = (SQLULEN) (digits + 27);

            break;
          }
        case bind::rowversion:
          {
            buf = (SQLPOINTER) b->buffer;
            col_size = 8;
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
          i,
          SQL_PARAM_INPUT,
          c_type_lookup[b->type],
          sql_type_lookup[b->type],
          col_size,
          digits,
          buf,
          0, // buffer capacity (shouldn't be needed for input parameters)
          b->size_ind);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
      }
    }

    SQLUSMALLINT statement::
    bind_result (bind* b, size_t n, SQLUSMALLINT& long_count)
    {
      long_count = 0;
      SQLRETURN r;

      SQLUSMALLINT i (0);
      for (bind* end (b + n); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

        SQLLEN cap (capacity_lookup[b->type]);

        switch (b->type)
        {
        case bind::string:
        case bind::nstring:
        case bind::binary:
          {
            cap = b->capacity;
            break;
          }
        case bind::long_string:
        case bind::long_nstring:
        case bind::long_binary:
          {
            // Long data is not bound.
            //
            long_count++;
            continue;
          }
        case bind::last:
          {
            assert (false);
            break;
          }
        default:
          break;
        }

        r = SQLBindCol (stmt_,
                        ++i, // Column index is 1-based.
                        c_type_lookup[b->type],
                        (SQLPOINTER) b->buffer,
                        cap,
                        b->size_ind);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
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
        details::buffer& tmp_buf (conn_.long_data_buffer ());

        if (tmp_buf.capacity () == 0)
          tmp_buf.capacity (4096);

        long_callback* pcb;
        for (;;)
        {
          // ODBC seems to already offset the returned pointer for us
          // in case of a batch.
          //
          r = SQLParamData (stmt_, (SQLPOINTER*) &pcb);

          // If we get anything other than SQL_NEED_DATA, then this is
          // the return code of SQLExecute().
          //
          if (r != SQL_NEED_DATA)
            break;

          // Store the pointer to the long_callback struct in buf on the
          // first call to the callback. This allows the callback to
          // redirect further calls to some other callback.
          //
          long_callback cb (*pcb);
          const void* buf (&cb);

          size_t position (0);
          for (;;)
          {
            size_t n;
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
              (SQLPOINTER) (buf != 0 ? buf : &buf), // Always pass non-NULL.
              chunk != chunk_null ? (SQLLEN) n : SQL_NULL_DATA);

            if (!SQL_SUCCEEDED (r))
              translate_error (r, conn_, stmt_);

            if (chunk == chunk_one ||
                chunk == chunk_last ||
                chunk == chunk_null)
              break;
          }
        }
      }

      return r;
    }

    void statement::
    stream_result (SQLUSMALLINT i, bind* b, size_t n, void* obase, void* nbase)
    {
      details::buffer& tmp_buf (conn_.long_data_buffer ());

      if (tmp_buf.capacity () == 0)
        tmp_buf.capacity (4096);

      SQLRETURN r;

      for (bind* end (b + n); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

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

        void* cbp;

        if (obase == 0)
          cbp = b->buffer;
        else
        {
          // Re-base the pointer.
          //
          char* p (static_cast<char*> (b->buffer));
          char* ob (static_cast<char*> (obase));
          char* nb (static_cast<char*> (nbase));

          assert (ob <= p);
          cbp = nb + (p - ob);
        }

        long_callback cb (*static_cast<long_callback*> (cbp));

        // First determine if the value is NULL as well as try to
        // get the total data size.
        //
        SQLLEN si;
        r = SQLGetData (stmt_,
                        ++i,
                        c_type_lookup[b->type],
                        tmp_buf.data (), // Dummy value.
                        0,
                        &si);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        // Store the pointer to the long_callback struct in buf on the
        // first call to the callback. This allows the callback to
        // redirect further calls to some other callback.
        //
        void* buf (&cb);
        size_t size (0);
        size_t position (0);
        size_t size_left (si == SQL_NO_TOTAL ? 0 : static_cast<size_t> (si));

        chunk_type c (si == SQL_NULL_DATA
                      ? chunk_null
                      : (si == 0 ? chunk_one : chunk_first));

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

          if (c == chunk_last || c == chunk_one || c == chunk_null)
            break;

          // SQLGetData() can keep returning SQL_SUCCESS_WITH_INFO (truncated)
          // with SQL_NO_TOTAL for all the calls except the last one. For the
          // last call we should get SQL_SUCCESS and the size_indicator should
          // contain a valid value.
          //
          r = SQLGetData (stmt_,
                          i,
                          c_type_lookup[b->type],
                          (SQLPOINTER) buf,
                          (SQLLEN) size,
                          &si);

          if (r == SQL_SUCCESS)
          {
            assert (si != SQL_NO_TOTAL);

            // Actual amount of data copied to the buffer (appears not to
            // include the NULL terminator).
            //
            size = static_cast<size_t> (si);
            c = chunk_last;
          }
          else if (r == SQL_SUCCESS_WITH_INFO)
          {
            if (char_data)
              size--; // NULL terminator.

            c = chunk_next;
          }
          else
            translate_error (r, conn_, stmt_);

          // Update the total.
          //
          if (size_left != 0)
            size_left -= size;
        }
      }
    }

    //
    // bulk_statement
    //
    bulk_statement::
    ~bulk_statement () {}

    void bulk_statement::
    init (size_t skip)
    {
      // Setup row-wise batch operation. We set the actual number of
      // parameter sets in the batch in execute().
      //
      SQLRETURN r;

      r = SQLSetStmtAttr (stmt_,
                          SQL_ATTR_PARAM_BIND_TYPE,
                          (SQLPOINTER) skip,
                          0);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      r = SQLSetStmtAttr (stmt_,
                          SQL_ATTR_PARAMS_PROCESSED_PTR,
                          (SQLPOINTER) &processed_,
                          0);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      r = SQLSetStmtAttr (stmt_,
                          SQL_ATTR_PARAM_STATUS_PTR,
                          (SQLPOINTER) status_,
                          0);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);
    }

    SQLRETURN bulk_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      mex_ = mex;

      if (status_ != 0)
      {
        SQLRETURN r (SQLSetStmtAttr (stmt_,
                                     SQL_ATTR_PARAMSET_SIZE,
                                     (SQLPOINTER) n,
                                     0));

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        // Some SQL* functions would only update the status in case of
        // an error.
        //
        memset (status_, 0, sizeof (status_[0]) * n);
      }

      processed_ = 0;
      SQLRETURN r (statement::execute ());
      bool ok (SQL_SUCCEEDED (r) || r == SQL_NO_DATA);

      // If we have a batch of 1 parameter set, SQL Server ODBC driver
      // returns the error via SQLExecute() rather than via the status
      // array even if we set all the attributes necessary for row-wise
      // binding. So what we are going to do here is convert this case
      // to the batch way of reporting errors (not that we also check
      // processed_ so that we only do this is the parameter set was
      // actually attempted).
      //
      if (!ok && status_ != 0 && n == 1 && processed_ == 1)
      {
        status_[0] = SQL_PARAM_ERROR;
        r = SQL_SUCCESS;
        ok = true;
      }

      // If the statement failed as a whole, assume no parameter sets
      // were attempted in case of a batch. Otherwise, the documentation
      // says that the native client driver keeps processing remaining
      // sets even in case of an error.
      //
      i_ = 0;
      n_ = (ok ? n : (status_ == 0 ? 1 : 0));

      if (mex_ != 0)
      {
        mex_->current (i_);
        mex_->attempted (processed_);
      }

      if (!ok)
      {
        if (mex_ != 0)
          mex_->fatal (true); // An incomplete batch is always fatal.

        return r;
      }

      return r;
    }

    size_t bulk_statement::
    extract_errors ()
    {
      size_t e (0);

      for (size_t i (0); i != n_; ++i)
      {
        if (status_[i] != SQL_PARAM_SUCCESS &&
            status_[i] != SQL_PARAM_SUCCESS_WITH_INFO)
        {
          translate_error (SQL_ERROR, conn_, stmt_, i, mex_);
          e++;
        }
      }

      return e;
    }

    unsigned long long bulk_statement::
    affected (SQLRETURN r, size_t errors, bool unique)
    {
      unsigned long long rows (0);

      // SQL_NO_DATA indicates that the statement hasn't affected any rows.
      //
      if (r != SQL_NO_DATA)
      {
        SQLLEN n;
        r = SQLRowCount (stmt_, &n);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        // If all the parameter sets failed, then the returned count is -1,
        // which means "not available" according to the documentation.
        //
        rows = (n != -1 ? static_cast<unsigned long long> (n) : 0);
      }

      if (n_ > 1) // Batch.
      {
        if (rows != 0) // Some rows did get affected.
        {
          // Subtract the parameter sets that failed since they haven't
          // affected any rows.
          //
          size_t p (n_ - errors);

          if (p > 1) // True batch.
          {
            if (unique) // Each can affect 0 or 1 row.
            {
              rows = (p == static_cast<size_t> (rows)
                      ? 1
                      : result_unknown);
            }
            else
              rows = result_unknown;
          }
        }
      }

      return rows;
    }

    //
    // select_statement
    //
    select_statement::
    ~select_statement ()
    {
    }

    select_statement::
    select_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      bool optimize,
                      binding& param,
                      binding& result)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize),
          result_ (result)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);
        result_count_ = bind_result (result.bind, result.count, long_count_);
      }
    }

    select_statement::
    select_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      bool optimize,
                      binding& param,
                      binding& result,
                      bool copy_text)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize,
                     copy_text),
          result_ (result)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);
        result_count_ = bind_result (result.bind, result.count, long_count_);
      }
    }

    select_statement::
    select_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      bool optimize,
                      binding& result)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize),
          result_ (result)
    {
      if (!empty ())
        result_count_ = bind_result (result.bind, result.count, long_count_);
    }

    select_statement::
    select_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      bool optimize,
                      binding& result,
                      bool copy_text)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize,
                     copy_text),
          result_ (result)
    {
      if (!empty ())
        result_count_ = bind_result (result.bind, result.count, long_count_);
    }

    void select_statement::
    execute ()
    {
      SQLRETURN r (statement::execute ());

      // Skip empty result sets that seem to be added as a result of
      // executing DML statements in stored procedures (e.g., INSERT
      // INTO EXEC).
      //
      if (r == SQL_NO_DATA)
      {
        r = SQLMoreResults (stmt_);

        if (r == SQL_NO_DATA)
        {
          throw database_exception (
            0,
            "?????",
            "another result set expected after SQL_NO_DATA");
        }
      }

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);

      // Skip result sets that have no columns. These seem to be added
      // by DML statements that don't produce any result (e.g., EXEC).
      //
      for (columns_ = 0; columns_ == 0;)
      {
        {
          SQLSMALLINT c;
          r = SQLNumResultCols (stmt_, &c);

          if (!SQL_SUCCEEDED (r))
            translate_error (r, conn_, stmt_);

          columns_ = static_cast<SQLUSMALLINT> (c);
        }

        if (columns_ == 0)
        {
          r = SQLMoreResults (stmt_);

          if (r == SQL_NO_DATA)
            break;
          else if (!SQL_SUCCEEDED (r))
            translate_error (r, conn_, stmt_);
        }
      }

      // Make sure that the number of columns in the result returned by
      // the database matches the number that we expect. A common cause
      // of this assertion is a native view with a number of data members
      // not matching the number of columns in the SELECT-list.
      //
      assert (columns_ == result_count_ + long_count_);
    }

    select_statement::result select_statement::
    fetch ()
    {
      change_callback* cc (result_.change_callback);

      if (cc != 0 && cc->callback != 0)
        (cc->callback) (cc->context);

      // Don't bother calling SQLFetch() if there are no columns.
      //
      if (columns_ == 0)
        return no_data;
      else
      {
        SQLRETURN r (SQLFetch (stmt_));

        if (r == SQL_NO_DATA)
          return no_data;

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        return success;
      }
    }

    void select_statement::
    free_result ()
    {
      // Use SQLFreeStmt(SQL_CLOSE) instead of SQLCloseCursor() to avoid an
      // error if a cursor is already closed. This can happens, for example,
      // if we are trying to close the cursor after the transaction has been
      // committed (e.g., when destroying the query result) which also closes
      // the cursor.
      //
      SQLRETURN r (SQLFreeStmt (stmt_, SQL_CLOSE));

      if (!SQL_SUCCEEDED (r))
        translate_error (r, conn_, stmt_);
    }

    //
    // insert_statement
    //

    insert_statement::
    ~insert_statement ()
    {
    }

    insert_statement::
    insert_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      binding& param,
                      bool returning_id,
                      bool returning_version,
                      binding* returning)
        : bulk_statement (conn,
                          text, statement_insert,
                          (process ? &param : 0), false,
                          param.batch, param.skip, param.status),
          returning_id_ (returning_id),
          returning_version_ (returning_version),
          ret_ (returning)
    {
      bind_param (param.bind, param.count);

      if (ret_ != 0)
        init_result ();
    }

    insert_statement::
    insert_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param,
                      bool returning_id,
                      bool returning_version,
                      binding* returning,
                      bool copy_text)
        : bulk_statement (conn,
                          text, statement_insert,
                          (process ? &param : 0), false,
                          param.batch, param.skip, param.status,
                          copy_text),
          returning_id_ (returning_id),
          returning_version_ (returning_version),
          ret_ (returning)
    {
      bind_param (param.bind, param.count);

      if (ret_ != 0)
        init_result ();
    }

    template <typename T>
    inline T*
    offset (T* base, size_t count, size_t size)
    {
      return reinterpret_cast<T*> (
        reinterpret_cast<char*> (base) + count * size);
    }

    void insert_statement::
    init_result ()
    {
      // Figure out if we are using the OUTPUT clause or a batch of
      // INSERT and SELECT statements. The latter is used to work
      // around a bug in SQL Server 2005 that causes it to fail
      // on an INSERT statement with the OUTPUT clause if data
      // for one of the inserted columns is supplied at execution
      // (long data).
      //
      text_batch_ = (strstr (text_, "OUTPUT INSERTED.") == 0 &&
                     strstr (text_, "output inserted.") == 0);

      // It might seem logical to set up the array of results if this is a
      // batch (i.e., the SQL_ATTR_ROW_BIND_TYPE, SQL_ATTR_ROW_ARRAY_SIZE).
      // This won't work because what we are getting is multiple result
      // sets (each containing a single row) and not multiple rows. As a
      // result, the SQL Server ODBC driver will always store the data in
      // the first element of our array. A bit counter-intuitive.
      //
      // At the same time it would be conceptually cleaner to have the
      // returned data extracted into the batch array instead of always
      // the first element. This is also how other database runtimes (e.g.,
      // Oracle) behave. So what we are going to do here is emulate this
      // by making the ODBC driver store the data into the last element
      // of the batch array and then copying it into the right place
      // after processing each result set (see fetch() below).
      //
      SQLRETURN r;
      SQLUSMALLINT col (1);

      size_t last (ret_->batch - 1);

      if (returning_id_)
      {
        bind& b (ret_->bind[0]); // Auto id is the first element.

        r = SQLBindCol (stmt_,
                        col++,
                        c_type_lookup[b.type],
                        (SQLPOINTER) offset (b.buffer, last, ret_->skip),
                        capacity_lookup[b.type],
                        offset (b.size_ind, last, ret_->skip));

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
      }

      if (returning_version_)
      {
        bind& b (ret_->bind[ret_->count - 1]); // Version is the last element.

        r = SQLBindCol (stmt_,
                        col++,
                        c_type_lookup[b.type],
                        (SQLPOINTER) offset (b.buffer, last, ret_->skip),
                        capacity_lookup[b.type],
                        offset (b.size_ind, last, ret_->skip));

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
      }
    }

    size_t insert_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      // The batch INSERT works in two different ways, depending on
      // whether we have the OUTPUT clause. If there is no OUTPUT, then
      // all the parameter sets are processed inside the SQLExecute()
      // call. If, however, there is OUTPUT, then the sets are
      // processed one at a time as we consume the results with
      // the SQLMoreResults() call below. Thus we in effect have
      // two counts: the "processed so far" as set by the API
      // (SQL_ATTR_PARAMS_PROCESSED_PTR) and the "to be processed"
      // (value in n_). Note that in the OUTPUT case if there is an
      // error, the processed count seems to jump by 2 for some reason.
      //
      // The OUTPUT case can be handled in two different ways: we can
      // "execute" (with SQLMoreResults()) each set as the user moves
      // from one result to the next (result() call). The advantage of
      // this approach is that the returned data ends up in the right
      // place automatically. The potential drawback is that the total
      // affected row count will only be available at the end. As a
      // result, this approach probably won't work if we need to handle,
      // say, UPDATE with OUTPUT (SQLRowCount() does not return an
      // intermediate total, at least not for INSERT).
      //
      // The alternative implementation would call SQLMoreResults()
      // inside execute() until all the parameter sets are executed.
      // In this case we will have to copy the extracted data into
      // the right place in the bindings (or update the binding before
      // each call to SQLMoreResults()). It is also not clear whether
      // the diagnostic records for the failed sets would accumulate.
      // If not, those will have to be stashed into mex on each
      // iteration.
      //
      SQLRETURN r (bulk_statement::execute (n, mex));

      // Statement failed as a whole, assume no parameter sets were
      // attempted in case of a batch.
      //
      if (!SQL_SUCCEEDED (r))
      {
        fetch (r);
        return n_;
      }

      if (status_ == 0) // Non-batch case.
        fetch (SQL_SUCCESS);
      else
        fetch (status_[i_] == SQL_PARAM_SUCCESS ||
               status_[i_] == SQL_PARAM_SUCCESS_WITH_INFO
               ? SQL_SUCCESS : SQL_ERROR);

      return n_;
    }

    void insert_statement::
    fetch (SQLRETURN r)
    {
      result_ = true;

      if (!SQL_SUCCEEDED (r))
      {
        // An auto-assigned object id should never cause a duplicate primary
        // key.
        //
        if (!returning_id_)
        {
          // Translate the integrity contraint violation (SQLSTATE 23000)
          // to the false result value. This code is similar to that found
          // in translate_error().
          //
          char sqlstate[SQL_SQLSTATE_SIZE + 1];
          SQLINTEGER native_code;
          SQLSMALLINT msg_size;

          bool cv (false);

          for (SQLSMALLINT i (1);; ++i)
          {
            SQLRETURN r;

            // Filter based on row association.
            //
            if (mex_ != 0)
            {
              SQLLEN n;
              r = SQLGetDiagField (SQL_HANDLE_STMT,
                                   stmt_,
                                   i,
                                   SQL_DIAG_ROW_NUMBER,
                                   &n,
                                   0,
                                   0);

              if (r == SQL_NO_DATA)
                break;
              else if (!SQL_SUCCEEDED (r))
                continue;

              if (n == SQL_NO_ROW_NUMBER ||
                  n == SQL_ROW_NUMBER_UNKNOWN ||
                  n != static_cast<SQLLEN> (i_ + 1)) // 1-based
                continue;
            }

            r= SQLGetDiagRecA (SQL_HANDLE_STMT,
                               stmt_,
                               i,
                               (SQLCHAR*) sqlstate,
                               &native_code,
                               0,
                               0,
                               &msg_size);

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
            else // SQLGetDiagRec() failure.
            {
              cv = false;
              break;
            }
          }

          if (cv)
            result_ = false;
        }

        if (result_)
        {
          translate_error (r, conn_, stmt_, i_, mex_); // Can return.
          result_ = false; // Prevent id/version extraction below or.
        }
      }

      // Fetch the row containing the id/version if this statement is
      // returning.
      //
      if (result_ && ret_ != 0)
      {
        if (text_batch_)
        {
          r = SQLMoreResults (stmt_);

          if (r == SQL_NO_DATA)
          {
            throw database_exception (
            0,
            "?????",
            "multiple result sets expected from a batch of statements");
          }
          else if (!SQL_SUCCEEDED (r))
            translate_error (r, conn_, stmt_);
        }

        r = SQLFetch (stmt_);

        if (r != SQL_NO_DATA && !SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);

        if (r == SQL_NO_DATA)
          throw database_exception (
            0,
            "?????",
            "result set expected from a statement with the OUTPUT clause");

        // See init_result() for details on what's going here.
        //
        size_t last (ret_->batch - 1);
        if (i_ != last)
        {
          if (returning_id_)
          {
            bind& b (ret_->bind[0]); // Auto id is the first element.

            memcpy (offset (b.buffer, i_, ret_->skip),
                    offset (b.buffer, last, ret_->skip),
                    capacity_lookup[b.type]);

            memcpy (offset (b.size_ind, i_, ret_->skip),
                    offset (b.size_ind, last, ret_->skip),
                    sizeof (*b.size_ind));
          }

          if (returning_version_)
          {
            bind& b (ret_->bind[ret_->count - 1]); // Version is the last.

            memcpy (offset (b.buffer, i_, ret_->skip),
                    offset (b.buffer, last, ret_->skip),
                    capacity_lookup[b.type]);

            memcpy (offset (b.size_ind, i_, ret_->skip),
                    offset (b.size_ind, last, ret_->skip),
                    sizeof (*b.size_ind));
          }
        }
      }
    }

    bool insert_statement::
    result (size_t i)
    {
      assert ((i_ == i || i_ + 1 == i) && i < n_);

      SQLRETURN r;

      // Get to the next result set if necessary.
      //
      if (i != i_)
      {
        mex_->current (++i_); // mex cannot be NULL since this is a batch.

        // Only in case of the OUTPUT clause do we have multiple result sets.
        //
        if (ret_ != 0)
        {
          r = SQLMoreResults (stmt_);

          // The actually processed count could have changed (see execute()).
          //
          mex_->attempted (processed_);

          if (r == SQL_NO_DATA)
          {
            throw database_exception (
              0,
              "?????",
              "multiple result sets expected from an array of parameters");
          }
        }

        fetch (status_[i_] == SQL_PARAM_SUCCESS ||
               status_[i_] == SQL_PARAM_SUCCESS_WITH_INFO
               ? SQL_SUCCESS : SQL_ERROR);
      }

      // Close the cursor if we are done.
      //
      if (ret_ != 0 && i_ + 1 == n_)
      {
        // Use SQLFreeStmt(SQL_CLOSE) instead of SQLCloseCursor() to avoid
        // an error if a cursor is not open. This seem to happen if the
        // statement failure was translated to a parameter set failure in
        // bulk_statement for batches of one.
        //
        r = SQLFreeStmt (stmt_, SQL_CLOSE);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, conn_, stmt_);
      }

      return result_;
    }

    //
    // update_statement
    //

    update_statement::
    ~update_statement ()
    {
    }

    update_statement::
    update_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      binding& param,
                      binding* returning)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.skip, param.status),
          unique_ (false),
          returning_ (returning != 0)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.
      init (param, returning);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const string& text,
                      bool unique,
                      bool process,
                      binding& param,
                      binding* returning)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.skip, param.status),
          unique_ (unique),
          returning_ (returning != 0)
    {
      init (param, returning);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param,
                      binding* returning,
                      bool copy_text)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.skip, param.status,
                          copy_text),
          unique_ (false),
          returning_ (returning != 0)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.
      init (param, returning);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* text,
                      bool unique,
                      bool process,
                      binding& param,
                      binding* returning,
                      bool copy_text)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.skip, param.status,
                          copy_text),
          unique_ (unique),
          returning_ (returning != 0)
    {
      init (param, returning);
    }

    void update_statement::
    init (binding& param, binding* ret)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);

        if (ret != 0)
        {
          bind& b (ret->bind[ret->count - 1]); // Version is the last element.

          SQLRETURN r (SQLBindCol (stmt_,
                                   1,
                                   c_type_lookup[b.type],
                                   (SQLPOINTER) b.buffer,
                                   capacity_lookup[b.type],
                                   b.size_ind));

          if (!SQL_SUCCEEDED (r))
            translate_error (r, conn_, stmt_);
        }
      }
    }

    size_t update_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      // In batch UPDATE without the OUTPUT clause (which is the
      // only kind we currently support) all the parameter sets
      // are processed inside SQLExecute() and the total count of
      // affected rows is available after it returns.
      //
      assert (!returning_ || status_ == 0);

      SQLRETURN r (bulk_statement::execute (n, mex));

      // Statement failed as a whole, assume no parameter sets were
      // attempted in case of a batch.
      //
      if (!(SQL_SUCCEEDED (r) || r == SQL_NO_DATA))
      {
        translate_error (r, conn_, stmt_, 0, mex_);
        return n_;
      }

      if (status_ == 0) // Non-batch case.
      {
        // Fetch the row containing the data if this statement is
        // returning. We still need to close the cursor even if we
        // haven't updated any rows.
        //
        if (returning_)
        {
          r = SQLFetch (stmt_);

          if (r != SQL_NO_DATA && !SQL_SUCCEEDED (r))
            translate_error (r, conn_, stmt_);

          // We have to get the result after fetching the OUTPUT data
          // but before closing the cursor.
          //
          result_ = affected (SQL_SUCCESS, 0, unique_);

          {
            SQLRETURN r (SQLCloseCursor (stmt_)); // Don't overwrite r.

            if (!SQL_SUCCEEDED (r))
              translate_error (r, conn_, stmt_);
          }

          if (result_ != 0 && r == SQL_NO_DATA)
            throw database_exception (
              0,
              "?????",
              "result set expected from a statement with the OUTPUT clause");
        }
        else
          result_ = affected (r, 0, unique_);
      }
      else
      {
        // Extract error information for failed parameter sets. If we do
        // this after calling SQLRowCount(), all the diagnostics records
        // that we need will be gone.
        //
        size_t errors (extract_errors ());

        // Figure out the affected row count.
        //
        result_ = affected (r, errors, unique_);
      }

      return n_;
    }

    //
    // delete_statement
    //

    delete_statement::
    ~delete_statement ()
    {
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const string& text,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.skip, param.status),
          unique_ (false)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.
      bind_param (param.bind, param.count);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const string& text,
                      bool unique,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.skip, param.status),
          unique_ (unique)
    {
      bind_param (param.bind, param.count);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* text,
                      binding& param,
                      bool copy_text)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.skip, param.status,
                          copy_text),
          unique_ (false)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.
      bind_param (param.bind, param.count);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* text,
                      bool unique,
                      binding& param,
                      bool copy_text)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.skip, param.status,
                          copy_text),
          unique_ (unique)
    {
      bind_param (param.bind, param.count);
    }

    size_t delete_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      // In batch DELETE without the OUTPUT clause (which is the
      // only kind we currently support) all the parameter sets
      // are processed inside SQLExecute() and the total count of
      // affected rows is available after it returns.
      //

      SQLRETURN r (bulk_statement::execute (n, mex));

      // Statement failed as a whole, assume no parameter sets were
      // attempted in case of a batch.
      //
      if (!(SQL_SUCCEEDED (r) || r == SQL_NO_DATA))
      {
        translate_error (r, conn_, stmt_, 0, mex_);
        return n_;
      }

      // Extract error information for failed parameter sets. If we do
      // this after calling SQLRowCount(), all the diagnostics records
      // that we need will be gone.
      //
      size_t errors (status_ != 0 ? extract_errors () : 0);

      // Figure out the affected row count.
      //
      result_ = affected (r, errors, unique_);

      return n_;
    }
  }
}
