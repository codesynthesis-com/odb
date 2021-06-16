// file      : odb/pgsql/statement.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/details/config.hxx> // ODB_CXX11

#include <libpq-fe.h>

#ifdef LIBPQ_HAS_PIPELINING
#  ifndef _WIN32
#    include <errno.h>
#    include <sys/select.h>
#  endif
#endif

#include <cstring> // strcmp
#include <utility> // pair
#include <cassert>

#ifdef ODB_CXX11
#  include <cstdlib> // strtoull
#else
#  include <sstream> // istringstream
#endif

#include <odb/tracer.hxx>

#include <odb/pgsql/pgsql-oid.hxx>
#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/exceptions.hxx>
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
#ifdef ODB_CXX11
        count = strtoull (s, 0, 10);
#else
        istringstream ss (s);
        ss >> count;
#endif
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

        deallocated_ = true;
        auto_handle<PGresult> h (PQexec (conn_.handle (), s.c_str ()));

        if (!is_good_result (h))
        {
          // When we try to execute an invalid statement, PG "poisons" the
          // transaction (those "current transaction is aborted, commands
          // ignored until end of transaction block" messages in the log).
          // This includes prepared statement deallocations (quite a stupid
          // decision, if you ask me).
          //
          // So what can happen in this situation is the deallocation fails
          // but we ignore it because we are already unwinding the stack
          // (i.e., the prepared statement execution has failed). Next the
          // user fixes things (e.g., passes valid parameters) and tries to
          // re-execute the same query. But since we have failed to deallocate
          // the statement, we now cannot re-prepare it; the name is already
          // in use.
          //
          // What can we do to fix this? One way would be to postpone the
          // deallocation until after the transaction is rolled back. This,
          // however, would require quite an elaborate machinery: connection
          // will have to store a list of such statements, etc. A much simpler
          // solution is to mark the connection as failed. While it maybe a
          // bit less efficient, we assume this is an "exceptional" situation
          // that doesn't occur often. The only potentially problematic case
          // is if the user holds the pointer to the connection and runs
          // multiple transactions on it. But in this case the user should
          // check if the connection is still good after each failure anyway.
          //
          conn_.mark_failed ();

          translate_error (conn_, h);
        }
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
          process_select (text_copy_,
                          text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '"', '"',
                          optimize);
          break;
        case statement_insert:
          process_insert (text_copy_,
                          text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '$');
          break;
        case statement_update:
          process_update (text_copy_,
                          text_,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '$');
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

    template <typename T>
    static inline T*
    offset (T* base, size_t count, size_t size)
    {
      return reinterpret_cast<T*> (
        reinterpret_cast<char*> (base) + count * size);
    }

    void statement::
    bind_param (native_binding& ns, const binding& bs, size_t pos)
    {
      assert (ns.count == bs.count);

      for (size_t i (0); i < ns.count; ++i)
      {
        const bind& b (bs.bind[i]);

        ns.formats[i] = 1;

        bool* n (b.is_null != 0 ? offset (b.is_null, pos, bs.skip) : 0);

        if ((n != 0 && *n) || b.buffer == 0) // Handle NULL entries.
        {
          ns.values[i] = 0;
          ns.lengths[i] = 0;
          continue;
        }

        ns.values[i] = static_cast<char*> (offset (b.buffer, pos, bs.skip));

        size_t l (0);

        switch (b.type)
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
        case bind::varbit:
          {
            // In this case b.buffer is a pointer to pointer to buffer so we
            // need to chase one level.
            //
            ns.values[i] = static_cast<char*> (
              *reinterpret_cast<void**> (ns.values[i]));
          }
          // Fall through.
        case bind::bit:
          {
            l = *offset (b.size, pos, bs.skip);
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

        ns.lengths[i] = static_cast<int> (l);
      }
    }

    bool statement::
    bind_result (const binding& bs,
                 PGresult* result,
                 size_t row,
                 bool truncated,
                 size_t pos)
    {
      bool r (true);
      int col_count (PQnfields (result));

      int col (0);
      for (size_t i (0); i != bs.count && col != col_count; ++i)
      {
        const bind& b (bs.bind[i]);

        if (b.buffer == 0) // Skip NULL entries.
          continue;

        int c (col++);

        {
          bool* t (b.truncated != 0 ? offset (b.truncated, pos, bs.skip) : 0);

          if (truncated && (t == 0 || !*t))
            continue;

          if (t != 0)
            *t = false;
        }

        // Check for NULL unless we are reloading a truncated result.
        //
        if (!truncated)
        {
          bool* n (offset (b.is_null, pos, bs.skip));

          *n = PQgetisnull (result, static_cast<int> (row), c) == 1;

          if (*n)
            continue;
        }

        void* buf (offset (b.buffer, pos, bs.skip));

        const char* v (PQgetvalue (result, static_cast<int> (row), c));

        switch (b.type)
        {
        case bind::boolean_:
          {
            *static_cast<bool*> (buf) = *reinterpret_cast<const bool*> (v);
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
            long long i (0);

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
                *static_cast<short*> (buf) =
                  endian_traits::hton (static_cast<short> (i));
                break;
              }
            case bind::integer:
              {
                *static_cast<int*> (buf) =
                  endian_traits::hton (static_cast<int> (i));
                break;
              }
            case bind::bigint:
              {
                *static_cast<long long*> (buf) = endian_traits::hton (i);
                break;
              }
            default:
              break;
            }

            break;
          }
        case bind::real:
          {
            *static_cast<float*> (buf) = *reinterpret_cast<const float*> (v);
            break;
          }
        case bind::double_:
          {
            *static_cast<double*> (buf) = *reinterpret_cast<const double*> (v);
            break;
          }
        case bind::date:
          {
            *static_cast<int*> (buf) = *reinterpret_cast<const int*> (v);
            break;
          }
        case bind::time:
        case bind::timestamp:
          {
            *static_cast<long long*> (buf) =
              *reinterpret_cast<const long long*> (v);
            break;
          }
        case bind::numeric:
        case bind::text:
        case bind::bytea:
        case bind::bit:
        case bind::varbit:
          {
            // Currently this is neither supported (due to capacity) nor used
            // in batches.
            //
#ifdef LIBPGSQL_EXTRA_CHECKS
            assert (pos == 0);
#endif

            *b.size = static_cast<size_t> (
              PQgetlength (result, static_cast<int> (row), c));

            if (b.capacity < *b.size)
            {
              if (b.truncated)
                *b.truncated = true;

              r = false;
              continue;
            }

            // In these cases b.buffer is a pointer to pointer to buffer so we
            // need to chase one level.
            //
            if (b.type != bind::bit)
              buf = *static_cast<void**> (buf);

            memcpy (buf, v, *b.size);
            break;
          }
        case bind::uuid:
          {
            memcpy (buf, v, 16);
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

#if defined(LIBPQ_HAS_PIPELINING) && !defined(_WIN32)

    // Note that this function always marks the connection as failed.
    //
    static void
    translate_connection_error (connection& conn)
    {
      const char* m (PQerrorMessage (conn.handle ()));

      if (PQstatus (conn.handle ()) == CONNECTION_BAD)
      {
        conn.mark_failed ();
        throw connection_lost ();
      }
      else
      {
        conn.mark_failed ();
        throw database_exception (m != 0 ? m : "bad connection state");
      }
    }

    // A RAII object for PGconn's non-blocking pipeline mode.
    //
    struct pipeline
    {
      connection& conn;
      int sock;

      explicit
      pipeline (connection& c)
          : conn (c)
      {
        PGconn* ch (conn.handle ());

        if ((sock = PQsocket (ch)) == -1   ||
            PQsetnonblocking (ch, 1) == -1 ||
            PQenterPipelineMode (ch) == 0)
        {
          translate_connection_error (conn);
        }
      }

      void
      close (bool throw_ = true)
      {
        if (!conn.failed ())
        {
          PGconn* ch (conn.handle ());

          if (PQexitPipelineMode (ch) == 0 ||
              PQsetnonblocking (ch, 0) == -1)
          {
            if (throw_)
              translate_connection_error (conn);
            else
              conn.mark_failed ();
          }
        }
      }

      ~pipeline ()
      {
        close (false);
      }

      pair<bool /* read */, bool /* write */>
      wait (bool write, bool throw_ = true)
      {
        fd_set wds;
        fd_set rds;

        for (;;)
        {
          if (write)
          {
            FD_ZERO (&wds);
            FD_SET (sock, &wds);
          }

          FD_ZERO (&rds);
          FD_SET (sock, &rds);

          if (select (sock + 1, &rds, write ? &wds : 0, 0, 0) != -1)
            break;

          if (errno != EINTR)
          {
            if (throw_)
              translate_connection_error (conn);
            else
            {
              conn.mark_failed ();
              return pair<bool, bool> (false, false);
            }
          }
        }

        return pair<bool, bool> (FD_ISSET (sock, &rds),
                                 write && FD_ISSET (sock, &wds));
      }
    };

    // A RAII object for recovering from an error in a pipeline.
    //
    // Specifically, it reads and discards results until reaching
    // PGRES_PIPELINE_SYNC.
    //
    struct pipeline_recovery
    {
      pipeline_recovery (pipeline& pl, bool wdone, bool sync)
          : pl_ (&pl), wdone_ (wdone), sync_ (sync)
      {
      }

      ~pipeline_recovery ()
      {
        if (pl_ != 0 && !pl_->conn.failed ())
        {
          PGconn* ch (pl_->conn.handle ());

          // This code runs as part of stack unwinding caused by an exception
          // so if we encounter an error, we "upgrade" the existing exception
          // by marking the connection as failed.
          //
          // The rest is essentially a special version of execute() below.
          //
          // Note that on the first iteration we may still have results from
          // the previous call to PQconsumeInput() (and these results may
          // be the entire outstanding sequence, in which case calling wait()
          // will block indefinitely).
          //
          for (bool first (true);; first = false)
          {
            if (sync_)
            {
              assert (!wdone_);

              if (PQpipelineSync (ch) == 0)
                break;

              sync_ = false;
            }

            pair<bool, bool> r (false, false);

            if (!first)
            {
              r = pl_->wait (!wdone_);
              if (!r.first && !r.second)
                break;
            }

            if (r.first /* read */ || first)
            {
              if (r.first && PQconsumeInput (ch) == 0)
                break;

              while (PQisBusy (ch) == 0)
              {
                auto_handle<PGresult> res (PQgetResult (ch));

                // We should only get NULLs as well as PGRES_PIPELINE_ABORTED
                // finished with PGRES_PIPELINE_SYNC.
                //
                if (res != 0)
                {
                  ExecStatusType stat (PQresultStatus (res));

                  if (stat == PGRES_PIPELINE_SYNC)
                    return;

                  assert (stat == PGRES_PIPELINE_ABORTED);
                }
              }
            }

            if (r.second /* write */)
            {
              int r (PQflush (ch));
              if (r == -1)
                break;

              if (r == 0)
                wdone_ = true;
            }
          }

          pl_->conn.mark_failed ();
        }
      }

      void
      cancel ()
      {
        pl_ = 0;
      }

    private:
      pipeline* pl_;
      bool wdone_;
      bool sync_;
    };

    size_t statement::
    execute (const binding& param,
             native_binding& native_param,
             size_t n,
             multiple_exceptions& mex,
             bool (*process) (size_t, PGresult*, bool, void*),
             void* data)
    {
      size_t i (0); // Parameter set being attempted.
      mex.current (i);

      PGconn* ch (conn_.handle ());

      pipeline pl (conn_);

      // True if we've written and read everything, respectively.
      //
      bool wdone (false), rdone (false);

      for (size_t wn (0), rn (0); !rdone; )
      {
        // Note that there is a special version of this code above in
        // ~pipeline_recovery().
        //
        pair<bool, bool> r (pl.wait (!wdone));

        // Note that once we start the pipeline, any call that may throw
        // without marking the connection as failed should be guarded by
        // pipeline_recovery.

        // Try to minimize the chance of blocking the server by first
        // processing the result and then sending more queries.
        //
        if (r.first /* read */)
        {
          if (PQconsumeInput (ch) == 0)
            translate_connection_error (conn_);

          while (PQisBusy (ch) == 0)
          {
            auto_handle<PGresult> res (PQgetResult (ch));

            ExecStatusType stat (PGRES_FATAL_ERROR);
            bool gr (is_good_result (res, &stat));

            if (stat == PGRES_PIPELINE_SYNC)
            {
              assert (wdone && rn == n);
              rdone = true;
              break;
            }

            assert (rn != n);
            ++rn;

            if (stat != PGRES_PIPELINE_ABORTED)
            {
              // translate_error() may throw an exception (e.g., deadlock)
              // without marking the connection as failed.
              //
              {
                pipeline_recovery plr (pl, wdone, wn != n);

                if (!process (i, res, gr, data))
                  translate_error (conn_, res, i, &mex);

                plr.cancel ();
              }

              mex.attempted (++i);
              mex.current (i);
            }
            else
            {
              // Should we treat PGRES_PIPELINE_ABORTED entries as attempted
              // or not? While we did issue PQsendQueryPrepared() for them,
              // the server tells us that it did not attemp to execute them.
              // So it feels like they should not be treated as attempted.
              //
              // Note that for this to fit into out multiple_exceptions model,
              // such an incomplete batch should be fatal (otherwise we could
              // end up with unattempted "holes"). This is currently the case
              // for errors handled by translate_error() but not necessarily
              // the case for those handled by the process function (e.g.,
              // duplicate id handled by process_insert_result() below). So in
              // a somewhat hackish way we assume the error (e.g., duplicate
              // id) will always be translated to an exception and pre-mark
              // multiple_exceptions as fatal.
              //
              mex.fatal (true);
            }

            // We get a NULL result after each query result.
            //
            {
              PGresult* end (PQgetResult (ch));
              assert (end == 0);
            }
          }
        }

        if (r.second /* write */)
        {
          // Send queries until we get blocked (write-biased). This feels like
          // a better overall strategy to keep the server busy compared to
          // sending one query at a time and then re-checking if there is
          // anything to read because the results of INSERT/UPDATE/DELETE are
          // presumably small and quite a few of them can get buffered before
          // the server gets blocked.
          //
          for (;;)
          {
            if (wn != n)
            {
              bind_param (native_param, param, wn);

              if (PQsendQueryPrepared (ch,
                                       name_,
                                       static_cast<int> (native_param.count),
                                       native_param.values,
                                       native_param.lengths,
                                       native_param.formats,
                                       1) == 0)
                translate_connection_error (conn_);

              if (++wn == n)
              {
                if (PQpipelineSync (ch) == 0)
                  translate_connection_error (conn_);
              }
            }

            // PQflush() result:
            //
            //  0  --  success (queue is now empty)
            //  1  --  blocked
            // -1  --  error
            //
            int r (PQflush (ch));
            if (r == -1)
              translate_connection_error (conn_);

            if (r == 0)
            {
              if (wn != n)
              {
                // If we continue here, then we are write-biased. And if we
                // break, then we are read-biased.
                //
#ifdef LIBPGSQL_READ_BIASED
                break;
#else
                continue;
#endif
              }

              wdone = true;
            }

            break; // Blocked or done.
          }
        }
      }

      pl.close ();
      return i;
    }
#endif

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
                        in ? static_cast<int> (native_param_->count) : 0,
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
      return bind_result (result_, handle_, current_row_ - 1)
        ? success
        : truncated;
    }

    void select_statement::
    reload ()
    {
      assert (current_row_ > 0);
      assert (current_row_ <= row_count_);

      if (!bind_result (result_, handle_, current_row_ - 1, true))
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
      if (returning_ != 0)
        assert (returning_->count == 1);
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
      if (returning_ != 0)
        assert (returning_->count == 1);
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
                        static_cast<int> (native_param_.count),
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
          const char* ss (PQresultErrorField (h, PG_DIAG_SQLSTATE));

          if (ss != 0 && strcmp (ss, "23505") == 0)
            return false;
        }

        translate_error (conn_, h);
      }

      if (returning_ != 0)
        bind_result (*returning_, h, 0);

      return true;
    }

#if defined(LIBPQ_HAS_PIPELINING) && !defined(_WIN32)

    struct insert_data
    {
      binding& param;
      binding* returning;
    };

    static bool
    process_insert_result (size_t i, PGresult* r, bool gr, void* data)
    {
      insert_data& d (*static_cast<insert_data*> (data));

      unsigned long long& s (d.param.status[i]);
      s = 1;

      if (gr)
      {
        // Note that the result can never be truncated.
        //
        if (d.returning != 0)
          statement::bind_result (*d.returning, r, 0, false, i);
      }
      else
      {
        // An auto-assigned object id should never cause a duplicate
        // primary key.
        //
        if (d.returning == 0 &&
            r != 0 && PQresultStatus (r) == PGRES_FATAL_ERROR)
        {
          // Note that statement::execute() assumes that this will eventually
          // be translated to an entry in multiple_exceptions.
          //
          const char* ss (PQresultErrorField (r, PG_DIAG_SQLSTATE));

          if (ss != 0 && strcmp (ss, "23505") == 0)
            s = 0;
        }

        if (s == 1)
          return false;
      }

      return true;
    }

    size_t insert_statement::
    execute (size_t n, multiple_exceptions& mex)
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      insert_data d {param_, returning_};

      return statement::execute (
        param_, native_param_, n, mex, &process_insert_result, &d);
    }
#endif

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
                        static_cast<int> (native_param_.count),
                        native_param_.values,
                        native_param_.lengths,
                        native_param_.formats,
                        1));

      if (!is_good_result (h))
        translate_error (conn_, h);

      return affected_row_count (h);
    }

#if defined(LIBPQ_HAS_PIPELINING) && !defined(_WIN32)

    static bool
    process_update_result (size_t i, PGresult* r, bool gr, void* data)
    {
      binding& param (*static_cast<binding*> (data));

      unsigned long long& s (param.status[i]);

      if (gr)
      {
        s = affected_row_count (r);
        return true;
      }
      else
      {
        s = update_statement::result_unknown;
        return false;
      }
    }

    size_t update_statement::
    execute (size_t n, multiple_exceptions& mex)
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      return statement::execute (
        param_, native_param_, n, mex, &process_update_result, &param_);
    }
#endif

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
                        static_cast<int> (native_param_.count),
                        native_param_.values,
                        native_param_.lengths,
                        native_param_.formats,
                        1));

      if (!is_good_result (h))
        translate_error (conn_, h);

      return affected_row_count (h);
    }

#if defined(LIBPQ_HAS_PIPELINING) && !defined(_WIN32)

    static bool
    process_delete_result (size_t i, PGresult* r, bool gr, void* data)
    {
      binding& param (*static_cast<binding*> (data));

      unsigned long long& s (param.status[i]);

      if (gr)
      {
        s = affected_row_count (r);
        return true;
      }
      else
      {
        s = delete_statement::result_unknown;
        return false;
      }
    }

    size_t delete_statement::
    execute (size_t n, multiple_exceptions& mex)
    {
      assert (param_ != 0);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      return statement::execute (
        *param_, native_param_, n, mex, &process_delete_result, param_);
    }
#endif
  }
}
