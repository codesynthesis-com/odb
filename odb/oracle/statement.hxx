// file      : odb/oracle/statement.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_STATEMENT_HXX
#define ODB_ORACLE_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/statement.hxx>
#include <odb/exceptions.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/binding.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/oracle-fwd.hxx>
#include <odb/oracle/auto-handle.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class LIBODB_ORACLE_EXPORT statement: public odb::statement
    {
    public:
      typedef oracle::connection connection_type;

      virtual
      ~statement () = 0;

      OCIStmt*
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
                 bool optimize);

    private:
      void
      init (const char* text,
            std::size_t text_size,
            statement_kind,
            const binding* process,
            bool optimize);

    protected:
      struct unbind
      {
        oracle::bind::buffer_type type; // Bind type.
        oracle::bind* bind;             // Corresponding bind entry.
        void* value;                    // Actual value passed to OCIBindByPos.
      };

      // Bind parameters for this statement. This function must only
      // be called once. Multiple calls to it will result in memory
      // leaks due to lost OCIBind resources. Return the actual number
      // of columns bound.
      //
      ub4
      bind_param (bind*, std::size_t count,
                  size_t batch = 1, std::size_t skip = 0);

      // Bind results for this statement. This function must only be
      // called once. Multiple calls to it will result in memory leaks
      // due to lost OCIDefine resources. Return the actual number of
      // columns bound.
      //
      ub4
      bind_result (bind*,
                   std::size_t count,
                   std::size_t lob_prefetch_size = 0);

      // Stream the result LOBs, calling user callbacks where necessary.
      // The old_base and new_base arguments can be used to "re-base" the
      // lob_callback struct pointer (stored in bind::callback), the lob
      // struct pointer (stored in bind::buffer), and the indicator value
      // pointer (stored in bind::indicator). This is used by the query
      // machinery to cause stream_result() to use the callback information
      // from a copy of the image instead of the bound image.
      //
      void
      stream_result (bind*,
                     std::size_t count,
                     void* old_base = 0,
                     void* new_base = 0);

    protected:
      connection_type& conn_;
      auto_handle<OCIStmt> stmt_;

      unbind* udata_;
      std::size_t usize_;
    };

    class LIBODB_ORACLE_EXPORT bulk_statement: public statement
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
                      sb4* status);

      bulk_statement (connection_type&,
                      const char* text,
                      statement_kind,
                      const binding* process,
                      bool optimize,
                      std::size_t batch,
                      sb4* status);

      // Call OCIStmtExecute() and set up the batch tracking variables (see
      // below). The ignore_code argument specifies optional error code that
      // should not be treated as an error.
      //
      sword
      execute (std::size_t n, multiple_exceptions*, sb4 ignore_code = 0);

      static const unsigned long long result_unknown = ~0ULL;

      unsigned long long
      affected (bool unique);

    protected:
      auto_handle<OCIError> err1_;
      sb4* status_;                // Parameter sets status array.
      std::size_t n_;              // Actual batch size.
      std::size_t i_;              // Position in result.
      std::size_t errors_;         // Number of parameter sets that failed.
      multiple_exceptions* mex_;
    };

    class LIBODB_ORACLE_EXPORT generic_statement: public statement
    {
    public:
      virtual
      ~generic_statement ();

      generic_statement (connection_type&, const std::string& text);
      generic_statement (connection_type&, const char* text);

      unsigned long long
      execute ();

    private:
      generic_statement (const generic_statement&);
      generic_statement& operator= (const generic_statement&);

    private:
      void
      init ();

    private:
      ub2 stmt_type_;
      bool bound_;
    };

    class LIBODB_ORACLE_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      select_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& param,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& param,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      enum result
      {
        success,
        no_data
      };

      void
      execute ();

      result
      fetch ();

      void
      stream_result (void* old_base = 0, void* new_base = 0)
      {
        statement::stream_result (result_.bind,
                                  result_.count,
                                  old_base,
                                  new_base);
      }

      void
      free_result ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      binding& result_;
      ub4 result_count_; // Actual number of bound columns.
      bool done_;
    };

    struct LIBODB_ORACLE_EXPORT auto_result
    {
      explicit auto_result (select_statement& s): s_ (s) {}
      ~auto_result () {s_.free_result ();}

    private:
      auto_result (const auto_result&);
      auto_result& operator= (const auto_result&);

    private:
      select_statement& s_;
    };

    class LIBODB_ORACLE_EXPORT insert_statement: public bulk_statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        binding& param,
                        binding* returning);

      insert_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        binding& param,
                        binding* returning);

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions& mex)
      {
        return execute (n, &mex);
      }

      // Return true if successful and false if this row is a duplicate.
      // All other errors are reported via exceptions.
      //
      bool
      result (std::size_t i)
      {
        // Get to the next parameter set if necessary.
        //
        if (i != i_)
        {
          mex_->current (++i_); // mex cannot be NULL since this is a batch.
          fetch (status_[i_] == 0 ? 0 /*OCI_SUCCESS*/ : -1 /*OCI_ERROR*/,
                 status_[i_]);
        }

        return result_;
      }

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
      init (binding& param);

      std::size_t
      execute (std::size_t, multiple_exceptions*);

      void
      fetch (sword r, sb4 code);

    public: // For odb_oracle_returning_*().
      binding* ret_;
      ub4 ret_size_; // You don't want to know (see statement.cxx).
      ub2* ret_prev_;

    private:
      bool result_;
    };

    class LIBODB_ORACLE_EXPORT update_statement: public bulk_statement
    {
    public:
      virtual
      ~update_statement ();

      // OCI does not expose individual affected row counts for batch
      // operations. Instead, it adds them all up and returns a single
      // count. This is bad news for us.
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
                        bool process_text,
                        binding& param);

      update_statement (connection_type& conn,
                        const std::string& text,
                        bool unique_hint,
                        bool process_text,
                        binding& param);

      update_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        binding& param);

      update_statement (connection_type& conn,
                        const char* text,
                        bool unique_hint,
                        bool process_text,
                        binding& param);

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
      std::size_t
      execute (std::size_t, multiple_exceptions*);

    private:
      bool unique_;
      unsigned long long result_;
    };

    class LIBODB_ORACLE_EXPORT delete_statement: public bulk_statement
    {
    public:
      virtual
      ~delete_statement ();

      // OCI does not expose individual affected row counts for batch
      // operations. Instead, it adds them all up and returns a single
      // count. This is bad news for us.
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
                        binding& param);

      delete_statement (connection_type& conn,
                        const char* text,
                        bool unique_hint,
                        binding& param);

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

#include <odb/oracle/statement.ixx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_STATEMENT_HXX
