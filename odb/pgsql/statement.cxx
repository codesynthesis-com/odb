// file      : odb/pgsql/statement.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstdlib> // std::atol
#include <cassert>
#include <sstream> // istringstream

#include <libpq-fe.h>

#include <odb/tracer.hxx>

#include <odb/pgsql/pgsql-oid.hxx>
#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/auto-handle.hxx>
#include <odb/pgsql/error.hxx>

#include <odb/pgsql/details/endian-traits.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    using namespace details;

    static unsigned long long
    affected_row_count (PGresult* h)
    {
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
      if (!deallocated_)
      {
        {
          odb::tracer* t;
          if ((t = conn_.transaction_tracer ()) ||
              (t = conn_.tracer ()) ||
              (t = conn_.database ().tracer ()))
            t->deallocate (conn_, *this);
        }

        string s ("deallocate \"");
        s += name_;
        s += "\"";

        auto_handle<PGresult> h (PQexec (conn_.handle (), s.c_str ()));
        deallocated_ = true;
      }
    }

    statement::
    statement (connection_type& conn,
               const string& name,
               const string& text,
               statement_kind sk,
               const binding* process,
               bool optimize,
               const Oid* types,
               size_t types_count)
        : conn_ (conn),
          name_copy_ (name), name_ (name_copy_.c_str ()),
          deallocated_ (false)
    {
      if (process == 0)
      {
        text_copy_ = text;
        text_ = text_copy_.c_str ();
      }
      else
        text_ = text.c_str (); // Temporary, see init().

      init (sk, process, optimize, types, types_count);

      //
      // If any code after this line throws, the statement will be leaked
      // (on the server) since deallocate() won't be called for it.
      //
    }

    statement::
    statement (connection_type& conn,
               const char* name,
               const char* text,
               statement_kind sk,
               const binding* process,
               bool optimize,
               bool copy,
               const Oid* types,
               size_t types_count)
        : conn_ (conn), deallocated_ (false)
    {
      if (copy)
      {
        name_copy_ = name;
        name_ = name_copy_.c_str ();
      }
      else
        name_ = name;

      if (process == 0 && copy)
      {
        text_copy_ = text;
        text_ = text_copy_.c_str ();
      }
      else
        text_ = text; // Potentially temporary, see init().

      init (sk, process, optimize, types, types_count);

      //
      // If any code after this line throws, the statement will be leaked
      // (on the server) since deallocate() won't be called for it.
      //
    }

    void statement::
    init (statement_kind sk,
          const binding* proc,
          bool optimize,
          const Oid* types,
          size_t types_count)
    {
      if (proc != 0)
      {
        switch (sk)
        {
        case statement_select:
          process_select (text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '"', '"',
                          optimize,
                          text_copy_);
          break;
        case statement_insert:
          process_insert (text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '$',
                          text_copy_);
          break;
        case statement_update:
          process_update (text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '$',
                          text_copy_);
          break;
        case statement_delete:
          assert (false);
        }

        text_ = text_copy_.c_str ();
      }

      // Empty statement.
      //
      if (*text_ == '\0')
      {
        deallocated_ = true;
        return;
      }

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->prepare (conn_, *this);
      }

      auto_handle<PGresult> h (
        PQprepare (conn_.handle (),
                   name_,
                   text_,
                   static_cast<int> (types_count),
                   types));

      if (!is_good_result (h))
        translate_error (conn_, h);

      //
      // If any code after this line throws, the statement will be leaked
      // (on the server) since deallocate() won't be called for it.
      //
    }

    const char* statement::
    text () const
    {
      return text_;
    }

    void statement::
    bind_param (native_binding& n, const binding& b)
    {
      assert (n.count == b.count);

      for (size_t i (0); i < n.count; ++i)
      {
        const bind& current_bind (b.bind[i]);

        n.formats[i] = 1;

        if (current_bind.buffer == 0 || // Skip NULL entries.
            (current_bind.is_null != 0 && *current_bind.is_null))
        {
          n.values[i] = 0;
          n.lengths[i] = 0;
          continue;
        }

        n.values[i] = static_cast<char*> (current_bind.buffer);

        size_t l (0);

        switch (current_bind.type)
        {
        case bind::boolean_:
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
      int col_count (PQnfields (result));

      int col (0);
      for (size_t i (0); i != count && col != col_count; ++i)
      {
        const bind& b (p[i]);

        if (b.buffer == 0) // Skip NULL entries.
          continue;

        int c (col++);

        if (truncated && (b.truncated == 0 || !*b.truncated))
          continue;

        if (b.truncated != 0)
          *b.truncated = false;

        // Check for NULL unless we are reloading a truncated result.
        //
        if (!truncated)
        {
          *b.is_null = PQgetisnull (result, static_cast<int> (row), c) == 1;

          if (*b.is_null)
            continue;
        }

        const char* v (PQgetvalue (result, static_cast<int> (row), c));

        switch (b.type)
        {
        case bind::boolean_:
          {
            *static_cast<bool*> (b.buffer) =
              *reinterpret_cast<const bool*> (v);
            break;
          }
        case bind::smallint:
        case bind::integer:
        case bind::bigint:
          {
            // If we are dealing with a custom schema, we may have a
            // difference in the integer widths. Note also that we have
            // to go to our endianness and back in order for casts to
            // work properly.
            //
            long long i;

            switch (PQftype (result, c))
            {
            case int2_oid:
              {
                i = endian_traits::ntoh (*reinterpret_cast<const short*> (v));
                break;
              }
            case int4_oid:
              {
                i = endian_traits::ntoh (*reinterpret_cast<const int*> (v));
                break;
              }
            case int8_oid:
              {
                i = endian_traits::ntoh (
                  *reinterpret_cast<const long long*> (v));
                break;
              }
            default:
              {
                assert (false); // Column in the database is not an integer.
                break;
              }
            }

            switch (b.type)
            {
            case bind::smallint:
              {
                *static_cast<short*> (b.buffer) =
                  endian_traits::hton (static_cast<short> (i));
                break;
              }
            case bind::integer:
              {
                *static_cast<int*> (b.buffer) =
                  endian_traits::hton (static_cast<int> (i));
                break;
              }
            case bind::bigint:
              {
                *static_cast<long long*> (b.buffer) = endian_traits::hton (i);
                break;
              }
            default:
              break;
            }

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
            *b.size = static_cast<size_t> (
              PQgetlength (result, static_cast<int> (row), c));

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

      // Make sure that the number of columns in the result returned by
      // the database matches the number that we expect. A common cause
      // of this assertion is a native view with a number of data members
      // not matching the number of columns in the SELECT-list.
      //
      assert (col == col_count);

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
    select_statement (connection_type& conn,
                      const std::string& name,
                      const std::string& text,
                      bool process,
                      bool optimize,
                      const Oid* types,
                      std::size_t types_count,
                      binding& param,
                      native_binding& native_param,
                      binding& result)
        : statement (conn,
                     name, text, statement_select,
                     (process ? &result : 0), optimize,
                     types, types_count),
          param_ (&param),
          native_param_ (&native_param),
          result_ (result),
          row_count_ (0),
          current_row_ (0)
    {
    }

    select_statement::
    select_statement (connection_type& conn,
                      const char* name,
                      const char* text,
                      bool process,
                      bool optimize,
                      const Oid* types,
                      std::size_t types_count,
                      binding& param,
                      native_binding& native_param,
                      binding& result,
                      bool copy)
        : statement (conn,
                     name, text, statement_select,
                     (process ? &result : 0), optimize, copy,
                     types, types_count),
          param_ (&param),
          native_param_ (&native_param),
          result_ (result),
          row_count_ (0),
          current_row_ (0)
    {
    }

    select_statement::
    select_statement (connection_type& conn,
                      const std::string& name,
                      const std::string& text,
                      bool process,
                      bool optimize,
                      binding& result)
        : statement (conn,
                     name, text, statement_select,
                     (process ? &result : 0), optimize,
                     0, 0),
          param_ (0),
          native_param_ (0),
          result_ (result),
          row_count_ (0),
          current_row_ (0)
    {
    }

    select_statement::
    select_statement (connection_type& conn,
                      const char* name,
                      const char* text,
                      bool process,
                      bool optimize,
                      binding& result,
                      bool copy)
        : statement (conn,
                     name, text, statement_select,
                     (process ? &result : 0), optimize, copy,
                     0, 0),
          param_ (0),
          native_param_ (0),
          result_ (result),
          row_count_ (0),
          current_row_ (0)
    {
    }

    select_statement::
    select_statement (connection_type& conn,
                      const std::string& name,
                      const std::string& text,
                      bool process,
                      bool optimize,
                      const Oid* types,
                      std::size_t types_count,
                      native_binding& native_param,
                      binding& result)
        : statement (conn,
                     name, text, statement_select,
                     (process ? &result : 0), optimize,
                     types, types_count),
          param_ (0),
          native_param_ (&native_param),
          result_ (result),
          row_count_ (0),
          current_row_ (0)
    {
    }

    void select_statement::
    execute ()
    {
      handle_.reset ();

      if (param_ != 0)
        bind_param (*native_param_, *param_);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      bool in (native_param_ != 0);

      handle_.reset (
        PQexecPrepared (conn_.handle (),
                        name_,
                        in ? native_param_->count : 0,
                        in ? native_param_->values : 0,
                        in ? native_param_->lengths : 0,
                        in ? native_param_->formats : 0,
                        1));

      if (!is_good_result (handle_))
        translate_error (conn_, handle_);

      row_count_ = static_cast<size_t> (PQntuples (handle_));
      current_row_ = 0;
    }

    void select_statement::
    free_result ()
    {
      handle_.reset ();
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
      return bind_result (result_.bind,
                          result_.count,
                          handle_,
                          current_row_ - 1)
        ? success
        : truncated;
    }

    void select_statement::
    reload ()
    {
      assert (current_row_ > 0);
      assert (current_row_ <= row_count_);

      if (!bind_result (result_.bind,
                        result_.count,
                        handle_,
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
    insert_statement (connection_type& conn,
                      const string& name,
                      const string& text,
                      bool process,
                      const Oid* types,
                      size_t types_count,
                      binding& param,
                      native_binding& native_param,
                      binding* returning)
        : statement (conn,
                     name, text, statement_insert,
                     (process ? &param : 0), false,
                     types, types_count),
          param_ (param),
          native_param_ (native_param),
          returning_ (returning)
    {
    }

    insert_statement::
    insert_statement (connection_type& conn,
                      const char* name,
                      const char* text,
                      bool process,
                      const Oid* types,
                      size_t types_count,
                      binding& param,
                      native_binding& native_param,
                      binding* returning,
                      bool copy)
        : statement (conn,
                     name, text, statement_insert,
                     (process ? &param : 0), false, copy,
                     types, types_count),
          param_ (param),
          native_param_ (native_param),
          returning_ (returning)
    {
    }

    bool insert_statement::
    execute ()
    {
      bind_param (native_param_, param_);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      auto_handle<PGresult> h (
        PQexecPrepared (conn_.handle (),
                        name_,
                        native_param_.count,
                        native_param_.values,
                        native_param_.lengths,
                        native_param_.formats,
                        1));

      ExecStatusType stat (PGRES_FATAL_ERROR);

      if (!is_good_result (h, &stat))
      {
        // An auto-assigned object id should never cause a duplicate
        // primary key.
        //
        if (returning_ == 0 && stat == PGRES_FATAL_ERROR)
        {
          string s (PQresultErrorField (h, PG_DIAG_SQLSTATE));

          if (s == "23505")
            return false;
        }

        translate_error (conn_, h);
      }

      if (returning_ != 0)
        bind_result (returning_->bind, 1, h, 0, false);

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
    update_statement (connection_type& conn,
                      const string& name,
                      const string& text,
                      bool process,
                      const Oid* types,
                      size_t types_count,
                      binding& param,
                      native_binding& native_param)
        : statement (conn,
                     name, text, statement_update,
                     (process ? &param : 0), false,
                     types, types_count),
          param_ (param),
          native_param_ (native_param)
    {
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* name,
                      const char* text,
                      bool process,
                      const Oid* types,
                      size_t types_count,
                      binding& param,
                      native_binding& native_param,
                      bool copy)
        : statement (conn,
                     name, text, statement_update,
                     (process ? &param : 0), false, copy,
                     types, types_count),
          param_ (param),
          native_param_ (native_param)
    {
    }

    unsigned long long update_statement::
    execute ()
    {
      bind_param (native_param_, param_);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      auto_handle<PGresult> h (
        PQexecPrepared (conn_.handle (),
                        name_,
                        native_param_.count,
                        native_param_.values,
                        native_param_.lengths,
                        native_param_.formats,
                        1));

      if (!is_good_result (h))
        translate_error (conn_, h);

      return affected_row_count (h);
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
                      const string& name,
                      const string& text,
                      const Oid* types,
                      size_t types_count,
                      binding& param,
                      native_binding& native_param)
        : statement (conn,
                     name, text, statement_delete,
                     0, false,
                     types, types_count),
          param_ (&param),
          native_param_ (native_param)
    {
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* name,
                      const char* text,
                      const Oid* types,
                      size_t types_count,
                      binding& param,
                      native_binding& native_param,
                      bool copy)
        : statement (conn,
                     name, text, statement_delete,
                     0, false, copy,
                     types, types_count),
          param_ (&param),
          native_param_ (native_param)
    {
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const string& name,
                      const string& text,
                      const Oid* types,
                      size_t types_count,
                      native_binding& native_param)
        : statement (conn,
                     name, text, statement_delete,
                     0, false,
                     types, types_count),
          param_ (0),
          native_param_ (native_param)
    {
    }

    unsigned long long delete_statement::
    execute ()
    {
      if (param_ != 0)
        bind_param (native_param_, *param_);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      auto_handle<PGresult> h (
        PQexecPrepared (conn_.handle (),
                        name_,
                        native_param_.count,
                        native_param_.values,
                        native_param_.lengths,
                        native_param_.formats,
                        1));

      if (!is_good_result (h))
        translate_error (conn_, h);

      return affected_row_count (h);
    }
  }
}
