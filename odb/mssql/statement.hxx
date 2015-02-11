// file      : odb/mssql/statement.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_STATEMENT_HXX
#define ODB_MSSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/statement.hxx>
#include <odb/exceptions.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/binding.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/auto-handle.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class connection;

    class LIBODB_MSSQL_EXPORT statement: public odb::statement
    {
    public:
      typedef mssql::connection connection_type;

      virtual
      ~statement () = 0;

      SQLHSTMT
      handle () const
      {
        return stmt_;
      }

      virtual const char*
      text () const;

      virtual connection_type&
      connection ()
      {
        return conn_;
      }

      // A statement can be empty. This is used to handle situations
      // where a SELECT or UPDATE statement ends up not having any
      // columns after processing. An empty statement cannot be
      // executed.
      //
      bool
      empty () const
      {
        return stmt_ == 0;
      }

    protected:
      // We keep two versions to take advantage of std::string COW.
      //
      statement (connection_type&,
                 const std::string& text,
                 statement_kind,
                 const binding* process,
                 bool optimize);

      statement (connection_type&,
                 const char* text,
                 statement_kind,
                 const binding* process,
                 bool optimize,
                 bool copy_text);

    private:
      void
      init (std::size_t text_size,
            statement_kind,
            const binding* process,
            bool optimize);

      // Custom implementation for SQL Server that also moves long data
      // columns to the end.
      //
      static void
      process_select (const char* statement,
                      const bind*,
                      std::size_t bind_size,
                      bool optimize,
                      std::string& result);

    protected:
      void
      bind_param (bind*, std::size_t count);

      // Return the actual number of columns bound.
      //
      SQLUSMALLINT
      bind_result (bind*, std::size_t count, SQLUSMALLINT& long_count);

      SQLRETURN
      execute ();

      // The old_base and new_base arguments can be used to "re-base"
      // the long_callback struct pointer (stored in bind::buffer).
      // This is used by the query machinery to cause stream_result()
      // to use the callback information from a copy of the image
      // instead of the bound image.
      //
      void
      stream_result (SQLUSMALLINT start_col,
                     bind*,
                     std::size_t count,
                     void* old_base = 0,
                     void* new_base = 0);

    protected:
      connection_type& conn_;
      std::string text_copy_;
      const char* text_;
      auto_handle<SQL_HANDLE_STMT> stmt_;
    };

    class LIBODB_MSSQL_EXPORT bulk_statement: public statement
    {
    public:
      virtual
      ~bulk_statement () = 0;

    protected:
      bulk_statement (connection_type&,
                      const std::string& text,
                      statement_kind,
                      const binding* process,
                      bool optimize,
                      std::size_t batch,
                      std::size_t skip,
                      SQLUSMALLINT* status);

      bulk_statement (connection_type&,
                      const char* text,
                      statement_kind,
                      const binding* process,
                      bool optimize,
                      std::size_t batch,
                      std::size_t skip,
                      SQLUSMALLINT* status,
                      bool copy_text);

      // Call SQLExecute() and set up the batch tracking variables (see
      // below). Note that this function does not treat SQL_NO_DATA as
      // an error since for DELETE and UPDATE statements this is a
      // shortcut notation for zero rows affected.
      //
      SQLRETURN
      execute (std::size_t n, multiple_exceptions*);

      // Return the number of failed parameter sets.
      //
      std::size_t
      extract_errors ();

      static const unsigned long long result_unknown = ~0ULL;

      unsigned long long
      affected (SQLRETURN, std::size_t errors, bool unique);

    private:
      void
      init (std::size_t skip);

    protected:
      SQLULEN processed_;    // Number of parameter sets processed so far.
      SQLUSMALLINT* status_; // Parameter sets status array.
      std::size_t n_;        // Actual batch size.
      std::size_t i_;        // Position in result.
      multiple_exceptions* mex_;
    };

    class LIBODB_MSSQL_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      // While the long data columns can appear in any order in the
      // result binding, they should appear last in the statement
      // text.
      //
      select_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& param,
                        binding& result);

      select_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& param,
                        binding& result,
                        bool copy_text = true);

      select_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& result);

      select_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& result,
                        bool copy_text = true);

      enum result
      {
        success,
        no_data
      };

      void
      execute ();

      result
      fetch ();

      // Return true if any long data was streamed.
      //
      bool
      stream_result (void* old_base = 0, void* new_base = 0)
      {
        if (long_count_ != 0)
          statement::stream_result (result_count_,
                                    result_.bind,
                                    result_.count,
                                    old_base,
                                    new_base);
        return long_count_ != 0;
      }

      void
      free_result ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      binding& result_;
      SQLUSMALLINT result_count_; // Actual number of columns bound.
      SQLUSMALLINT long_count_;   // Number of long data columns.
      SQLUSMALLINT columns_;      // Number of columns in result set.
    };

    struct LIBODB_MSSQL_EXPORT auto_result
    {
      explicit auto_result (select_statement& s): s_ (&s) {}
      ~auto_result () {free ();}

      // Extended interface to support delayed freeing.
      //
      auto_result (): s_ (0) {}

      void
      set (select_statement& s) {s_ = &s;}

      void
      free ()
      {
        if (s_ != 0)
        {
          s_->free_result ();
          s_ = 0;
        }
      }

      void
      release () {s_ = 0;}

    private:
      auto_result (const auto_result&);
      auto_result& operator= (const auto_result&);

    private:
      select_statement* s_;
    };

    class LIBODB_MSSQL_EXPORT insert_statement: public bulk_statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        binding& param,
                        bool returning_id,
                        bool returning_version,
                        binding* returning);

      insert_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        binding& param,
                        bool returning_id,
                        bool returning_version,
                        binding* returning,
                        bool copy_text = true);

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions& mex)
      {
        return execute (n, &mex);
      }

      // Return true if successful and false if this row is a duplicate.
      // All other errors are reported by throwing exceptions.
      //
      bool
      result (std::size_t i);

      bool
      execute ()
      {
        execute (1, 0);
        return result (0);
      }

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

    private:
      void
      init_result ();

      std::size_t
      execute (std::size_t, multiple_exceptions*);

      void
      fetch (SQLRETURN);

    private:
      bool returning_id_;
      bool returning_version_;
      binding* ret_;
      bool text_batch_;

      bool result_;
    };

    class LIBODB_MSSQL_EXPORT update_statement: public bulk_statement
    {
    public:
      virtual
      ~update_statement ();

      // SQL Server native client ODBC driver does not expose individual
      // affected row counts for batch operations, even though it says it
      // does (SQLGetInfo(SQL_PARAM_ARRAY_ROW_COUNTS) returns SQL_PARC_BATCH).
      // Instead, it adds them all up and returns a single count. This is
      // bad news for us.
      //
      // In case of updating by primary key (the affected row count is
      // either 1 or 0), we can recognize the presumably successful case
      // where the total affected row count is equal to the batch size
      // (we can also recognize the "all unsuccessful" case where the
      // total affected row count is 0). The unique_hint argument in the
      // constructors below indicates whether this is a "0 or 1" UPDATE
      // statement.
      //
      // In all other situations (provided this is a batch), the result()
      // function below returns the special result_unknown value.
      //
      update_statement (connection_type& conn,
                        const std::string& text,
                        bool process,
                        binding& param,
                        binding* returning);

      update_statement (connection_type& conn,
                        const std::string& text,
                        bool unique_hint,
                        bool process,
                        binding& param,
                        binding* returning);

      update_statement (connection_type& conn,
                        const char* text,
                        bool process,
                        binding& param,
                        binding* returning,
                        bool copy_text = true);

      update_statement (connection_type& conn,
                        const char* text,
                        bool unique_hint,
                        bool process,
                        binding& param,
                        binding* returning,
                        bool copy_text = true);

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions& mex)
      {
        return execute (n, &mex);
      }

      // Return the number of rows affected (deleted) by the parameter
      // set. If this is a batch (n > 1 in execute() call above) and it
      // is impossible to determine the affected row count for each
      // parameter set, then this function returns result_unknown. All
      // other errors are reported by throwing exceptions.
      //
      using bulk_statement::result_unknown;

      unsigned long long
      result (std::size_t i)
      {
        if (i != i_)
          mex_->current (++i_); // mex cannot be NULL since this is a batch.

        return result_;
      }

      unsigned long long
      execute ()
      {
        execute (1, 0);
        return result (0);
      }

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);

    private:
      void
      init (binding& param, binding* ret);

      std::size_t
      execute (std::size_t, multiple_exceptions*);

    private:
      bool unique_;
      bool returning_;

      unsigned long long result_;
    };

    class LIBODB_MSSQL_EXPORT delete_statement: public bulk_statement
    {
    public:
      virtual
      ~delete_statement ();

      // SQL Server native client ODBC driver does not expose individual
      // affected row counts for batch operations, even though it says it
      // does (SQLGetInfo(SQL_PARAM_ARRAY_ROW_COUNTS) returns SQL_PARC_BATCH).
      // Instead, it adds them all up and returns a single count. This is
      // bad news for us.
      //
      // In case of deleting by primary key (the affected row count is
      // either 1 or 0), we can recognize the presumably successful case
      // where the total affected row count is equal to the batch size
      // (we can also recognize the "all unsuccessful" case where the
      // total affected row count is 0). The unique_hint argument in the
      // constructors below indicates whether this is a "0 or 1" DELETE
      // statement.
      //
      // In all other situations (provided this is a batch), the result()
      // function below returns the special result_unknown value.
      //
      delete_statement (connection_type& conn,
                        const std::string& text,
                        binding& param);

      delete_statement (connection_type& conn,
                        const std::string& text,
                        bool unique_hint,
                        binding& param);

      delete_statement (connection_type& conn,
                        const char* text,
                        binding& param,
                        bool copy_text = true);

      delete_statement (connection_type& conn,
                        const char* text,
                        bool unique_hint,
                        binding& param,
                        bool copy_text = true);

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions& mex)
      {
        return execute (n, &mex);
      }

      // Return the number of rows affected (deleted) by the parameter
      // set. If this is a batch (n > 1 in execute() call above) and it
      // is impossible to determine the affected row count for each
      // parameter set, then this function returns result_unknown. All
      // other errors are reported by throwing exceptions.
      //
      using bulk_statement::result_unknown;

      unsigned long long
      result (std::size_t i)
      {
        if (i != i_)
          mex_->current (++i_); // mex cannot be NULL since this is a batch.

        return result_;
      }

      unsigned long long
      execute ()
      {
        execute (1, 0);
        return result (0);
      }

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);

    private:
      std::size_t
      execute (std::size_t, multiple_exceptions*);

    private:
      bool unique_;
      unsigned long long result_;
    };
  }
}

#include <odb/mssql/statement.ixx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_STATEMENT_HXX
