// file      : odb/mysql/statement.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstring> // std::strlen, std::memmove, std::memset
#include <cassert>

#include <odb/tracer.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/database.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/error.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
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
        text_ = text; // Potentially temporary, see init().
        n = strlen (text_);
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
          process_select (text_copy_,
                          text_,
                          &proc->bind->buffer, proc->count, sizeof (MYSQL_BIND),
                          '`', '`',
                          optimize);
          break;
        case statement_insert:
          process_insert (text_copy_,
                          text_,
                          &proc->bind->buffer, proc->count, sizeof (MYSQL_BIND),
                          '?');
          break;
        case statement_update:
          process_update (text_copy_,
                          text_,
                          &proc->bind->buffer, proc->count, sizeof (MYSQL_BIND),
                          '?');
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

      stmt_.reset (conn_.alloc_stmt_handle ());

      conn_.clear ();

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->prepare (conn_, *this);
      }

      if (mysql_stmt_prepare (stmt_,
                              text_,
                              static_cast<unsigned long> (text_size)) != 0)
        translate_error (conn_, stmt_);
    }

    size_t statement::
    process_bind (MYSQL_BIND* b, size_t n)
    {
      size_t shifts (0);
      for (MYSQL_BIND* e (b + n); b != e;)
      {
        if (b->buffer == 0)
        {
          // It is possible that this array has already been processed
          // (shared among multiple statements).
          //
          if (b->length != 0)
          {
            n -= e - b;
            break;
          }

          e--;

          // Shift the rest of the entries to the left.
          //
          memmove (b, b + 1, (e - b) * sizeof (MYSQL_BIND));

          // Store the original position of the NULL entry at the end.
          //
          e->buffer = 0;
          e->length = reinterpret_cast<unsigned long*> (b + shifts);

          shifts++;
          continue;
        }

        b++;
      }

      return n - shifts;
    }

    void statement::
    restore_bind (MYSQL_BIND* b, size_t n)
    {
      for (MYSQL_BIND* e (b + n - 1); e->buffer == 0 && e->length != 0;)
      {
        MYSQL_BIND* p (reinterpret_cast<MYSQL_BIND*> (e->length));

        // Shift the entries from the specified position to the right.
        //
        memmove (p + 1, p, (e - p) * sizeof (MYSQL_BIND));

        // Restore the original NULL entry.
        //
        memset (p, 0, sizeof (MYSQL_BIND));
      }
    }

    statement::
    ~statement ()
    {
      if (stmt_ != 0)
      {
        {
          odb::tracer* t;
          if ((t = conn_.transaction_tracer ()) ||
              (t = conn_.tracer ()) ||
              (t = conn_.database ().tracer ()))
            t->deallocate (conn_, *this);
        }

        // Let the connection handle the release of the statement (it
        // may delay the actual freeing if it will mess up the currently
        // active statement).
        //
        conn_.free_stmt_handle (stmt_);
      }
    }

    const char* statement::
    text () const
    {
      return text_;
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
      assert (freed_);
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
          end_ (false),
          cached_ (false),
          freed_ (true),
          rows_ (0),
          param_ (&param),
          param_version_ (0),
          result_ (result),
          result_version_ (0)
    {
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
          end_ (false),
          cached_ (false),
          freed_ (true),
          rows_ (0),
          param_ (&param),
          param_version_ (0),
          result_ (result),
          result_version_ (0)
    {
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
          end_ (false),
          cached_ (false),
          freed_ (true),
          rows_ (0),
          param_ (0),
          result_ (result),
          result_version_ (0)
    {
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
          end_ (false),
          cached_ (false),
          freed_ (true),
          rows_ (0),
          param_ (0),
          result_ (result),
          result_version_ (0)
    {
    }

    void select_statement::
    execute ()
    {
      assert (freed_);

      conn_.clear ();

      end_ = false;
      rows_ = 0;

      if (mysql_stmt_reset (stmt_))
        translate_error (conn_, stmt_);

      if (param_ != 0 && param_version_ != param_->version)
      {
        // For now cannot have NULL entries.
        //
        if (mysql_stmt_bind_param (stmt_, param_->bind))
          translate_error (conn_, stmt_);

        param_version_ = param_->version;
      }

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      if (mysql_stmt_execute (stmt_))
        translate_error (conn_, stmt_);

      // This flag appears to be cleared once we start processing the
      // result, so we have to cache it for free_result() below.
      //
#if MYSQL_VERSION_ID >= 50503
      out_params_ = (conn_.handle ()->server_status & SERVER_PS_OUT_PARAMS);
#endif

      freed_ = false;
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
            translate_error (conn_, stmt_);

          // mysql_stmt_num_rows() returns the number of rows that have been
          // fetched by store_result.
          //
          size_ = rows_ + static_cast<size_t> (mysql_stmt_num_rows (stmt_));
        }
        else
          size_ = rows_;

        cached_ = true;
      }
    }

    select_statement::result select_statement::
    fetch (bool next)
    {
      if (result_version_ != result_.version)
      {
        size_t count (process_bind (result_.bind, result_.count));

        // Make sure that the number of columns in the result returned by
        // the database matches the number that we expect. A common cause
        // of this assertion is a native view with a number of data members
        // not matching the number of columns in the SELECT-list.
        //
        assert (mysql_stmt_field_count (stmt_) == count);

        if (mysql_stmt_bind_result (stmt_, result_.bind))
          translate_error (conn_, stmt_);

        if (count != result_.count)
          restore_bind (result_.bind, result_.count);

        result_version_ = result_.version;
      }

      if (!next && rows_ != 0)
      {
        assert (cached_);
        mysql_stmt_data_seek (stmt_, static_cast<my_ulonglong> (rows_ - 1));
      }

      int r (mysql_stmt_fetch (stmt_));

      switch (r)
      {
      case 0:
        {
          if (next)
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
          if (next)
            rows_++;
          return truncated;
        }
      default:
        {
          translate_error (conn_, stmt_);
          return no_data; // Never reached.
        }
      }
    }

    void select_statement::
    refetch ()
    {
      // Re-fetch columns that were truncated.
      //
      unsigned int col (0);
      for (size_t i (0); i < result_.count; ++i)
      {
        MYSQL_BIND& b (result_.bind[i]);

        if (b.buffer == 0) // Skip NULL entries.
          continue;

        if (*b.error)
        {
          *b.error = 0;

          if (mysql_stmt_fetch_column (stmt_, &b, col, 0))
            translate_error (conn_, stmt_);
        }

        col++;
      }
    }

    void select_statement::
    free_result ()
    {
      if (!freed_)
      {
        // If this is a stored procedure call, then we have multiple
        // results. The first is the rowset that is the result of the
        // procedure (actually, it can be several rowsets if, for
        // example, the procedure executes multiple SELECT statements,
        // but we don't support this). This result we have just handled.
        // Next, there could be another rowset with just one row which
        // contains the output variable values (OUT and INOUT; actually,
        // if the procedure does not have any SELECT statements, then
        // this will be the first result and we have just handled it).
        // Finally, the last result is always the stored procedure status
        // (not clear how to get this value; all MySQL sample code simply
        // ignores it).
        //
        // So what we need to do here is read and ignore these other
        // results since otherwise MySQL won't let us execute any
        // subsequent statements. Calling mysql_stmt_next_result()
        // until it tells us there is no more results seems to do
        // the trick.
        //
        // mysql_stmt_next_result() is only available since 5.5.3.
        // Checking the source code reveals that it does call
        // mysql_next_result() (which is available prior to 5.5.3)
        // but also does some other house keeping. So it is not
        // clear if it is possible to emulate the below logic for
        // prior MySQL versions.
        //

        // Handling OUT parameters requires a special Voodoo dance:
        // we have to fetch the row itself and we have to call fetch
        // again and get MYSQL_NO_DATA. Without doing these exact
        // steps the server simply drops the connection. Go figure.
        //
#if MYSQL_VERSION_ID >= 50503
        if (out_params_)
        {
          if (mysql_stmt_fetch (stmt_) != MYSQL_NO_DATA)
            translate_error (conn_, stmt_);
        }
#endif

        if (mysql_stmt_free_result (stmt_))
          translate_error (conn_, stmt_);

#if MYSQL_VERSION_ID >= 50503
        {
          int s;
          while ((s = mysql_stmt_next_result (stmt_)) == 0)
          {
            if (mysql_stmt_field_count (stmt_) != 0)
            {
              // The same Voodoo dance as above.
              //
              if (conn_.handle ()->server_status & SERVER_PS_OUT_PARAMS)
              {
                if (mysql_stmt_fetch (stmt_))
                  translate_error (conn_, stmt_);

                if (mysql_stmt_fetch (stmt_) != MYSQL_NO_DATA)
                  translate_error (conn_, stmt_);
              }

              if (mysql_stmt_free_result (stmt_))
                translate_error (conn_, stmt_);
            }
          }

          if (s != -1)
            translate_error (conn_, stmt_);
        }
#endif

        if (conn_.active () == this)
          conn_.active (0);

        end_ = true;
        cached_ = false;
        freed_ = true;
        rows_ = 0;
      }
    }

    void select_statement::
    cancel ()
    {
      // If we cached the result, don't free it just yet.
      //
      if (!cached_ || end_)
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
    insert_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      binding& param,
                      binding* returning)
        : statement (conn,
                     text, statement_insert,
                     (process ? &param : 0), false),
          param_ (param),
          param_version_ (0),
          returning_ (returning)
    {
    }

    insert_statement::
    insert_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param,
                      binding* returning,
                      bool copy_text)
        : statement (conn,
                     text, statement_insert,
                     (process ? &param : 0), false,
                     copy_text),
          param_ (param),
          param_version_ (0),
          returning_ (returning)
    {
    }

    bool insert_statement::
    execute ()
    {
      conn_.clear ();

      if (mysql_stmt_reset (stmt_))
        translate_error (conn_, stmt_);

      if (param_version_ != param_.version)
      {
        size_t count (process_bind (param_.bind, param_.count));

        if (mysql_stmt_bind_param (stmt_, param_.bind))
          translate_error (conn_, stmt_);

        if (count != param_.count)
          restore_bind (param_.bind, param_.count);

        param_version_ = param_.version;
      }

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      if (mysql_stmt_execute (stmt_))
      {
        // An auto-assigned object id should never cause a duplicate
        // primary key.
        //
        if (returning_ == 0 && mysql_stmt_errno (stmt_) == ER_DUP_ENTRY)
          return false;
        else
          translate_error (conn_, stmt_);
      }

      if (returning_ != 0)
      {
        unsigned long long i (mysql_stmt_insert_id (stmt_));

        MYSQL_BIND& b (returning_->bind[0]);
        void* v (b.buffer);

        switch (b.buffer_type)
        {
        case MYSQL_TYPE_TINY:
          *static_cast<unsigned char*> (v) = static_cast<unsigned char> (i);
          break;
        case MYSQL_TYPE_SHORT:
          *static_cast<unsigned short*> (v) = static_cast<unsigned short> (i);
          break;
        case MYSQL_TYPE_LONG:
          *static_cast<unsigned int*> (v) = static_cast<unsigned int> (i);
          break;
        case MYSQL_TYPE_LONGLONG:
          *static_cast<unsigned long long*> (v) = i;
          break;
        default:
          assert (false); // Auto id column type is not an integer.
        }

        *b.is_null = false;
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
    update_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      binding& param)
        : statement (conn,
                     text, statement_update,
                     (process ? &param : 0), false),
          param_ (param),
          param_version_ (0)
    {
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param,
                      bool copy_text)
        : statement (conn,
                     text, statement_update,
                     (process ? &param : 0), false,
                     copy_text),
          param_ (param),
          param_version_ (0)
    {
    }

    unsigned long long update_statement::
    execute ()
    {
      conn_.clear ();

      if (mysql_stmt_reset (stmt_))
        translate_error (conn_, stmt_);

      if (param_version_ != param_.version)
      {
        size_t count (process_bind (param_.bind, param_.count));

        if (mysql_stmt_bind_param (stmt_, param_.bind))
          translate_error (conn_, stmt_);

        if (count != param_.count)
          restore_bind (param_.bind, param_.count);

        param_version_ = param_.version;
      }

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      if (mysql_stmt_execute (stmt_))
        translate_error (conn_, stmt_);

      my_ulonglong r (mysql_stmt_affected_rows (stmt_));

      if (r == static_cast<my_ulonglong> (-1))
        translate_error (conn_, stmt_);

      return static_cast<unsigned long long> (r);
    }

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
        : statement (conn,
                     text, statement_delete,
                     0, false),
          param_ (param),
          param_version_ (0)
    {
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* text,
                      binding& param,
                      bool copy_text)
        : statement (conn,
                     text, statement_delete,
                     0, false,
                     copy_text),
          param_ (param),
          param_version_ (0)
    {
    }

    unsigned long long delete_statement::
    execute ()
    {
      conn_.clear ();

      if (mysql_stmt_reset (stmt_))
        translate_error (conn_, stmt_);

      if (param_version_ != param_.version)
      {
        // Cannot have NULL entries for now.
        //
        if (mysql_stmt_bind_param (stmt_, param_.bind))
          translate_error (conn_, stmt_);

        param_version_ = param_.version;
      }

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      if (mysql_stmt_execute (stmt_))
        translate_error (conn_, stmt_);

      my_ulonglong r (mysql_stmt_affected_rows (stmt_));

      if (r == static_cast<my_ulonglong> (-1))
        translate_error (conn_, stmt_);

      return static_cast<unsigned long long> (r);
    }
  }
}
