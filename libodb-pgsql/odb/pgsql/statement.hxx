// file      : odb/pgsql/statement.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_STATEMENT_HXX
#define ODB_PGSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/statement.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/binding.hxx>
#include <odb/pgsql/pgsql-fwd.hxx> // PGresult
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/auto-handle.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class connection;

    class LIBODB_PGSQL_EXPORT statement: public odb::statement
    {
    public:
      typedef pgsql::connection connection_type;

      virtual
      ~statement () = 0;

      const char*
      name () const
      {
        return name_;
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
        return *text_ == '\0';
      }

      void
      deallocate ();

      // Adapt an ODB binding to a native PostgreSQL parameter binding. If pos
      // is not 0, then bind the parameter set at this position in a batch.
      //
      static void
      bind_param (native_binding&, const binding&, std::size_t pos = 0);

      // Populate an ODB binding given a PostgreSQL result. If the truncated
      // argument is true, then only truncated columns are extracted. Return
      // true if all the data was extracted successfully and false if one or
      // more columns were truncated. If pos is not 0, then populate the
      // parameter set at this position in a batch.
      //
      static bool
      bind_result (const binding&,
                   PGresult*,
                   std::size_t row,
                   bool truncated = false,
                   std::size_t pos = 0);

    protected:
      // We keep two versions to take advantage of std::string COW.
      //
      statement (connection_type&,
                 const std::string& name,
                 const std::string& text,
                 statement_kind,
                 const binding* process,
                 bool optimize,
                 const Oid* types,
                 std::size_t types_count);

      statement (connection_type&,
                 const char* name,
                 const char* text,
                 statement_kind,
                 const binding* process,
                 bool optimize,
                 bool copy_name_text,
                 const Oid* types,
                 std::size_t types_count);

      // Bulk execute implementation.
      //
      std::size_t
      execute (const binding& param,
               native_binding& native_param,
               std::size_t n,
               multiple_exceptions&,
               bool (*process) (size_t i, PGresult*, bool good, void* data),
               void* data);

    private:
      void
      init (statement_kind,
            const binding* process,
            bool optimize,
            const Oid* types,
            std::size_t types_count);

    protected:
      connection_type& conn_;

      std::string name_copy_;
      const char* name_;

      std::string text_copy_;
      const char* text_;

    private:
      bool deallocated_;
    };

    class LIBODB_PGSQL_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      select_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        binding& result);

      select_statement (connection_type& conn,
                        const char* name,
                        const char* stmt,
                        bool process_text,
                        bool optimize_text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        binding& result,
                        bool copy_name_text = true);

      select_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& result);

      select_statement (connection_type& conn,
                        const char* name,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& result,
                        bool copy_name_text = true);

      select_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        const Oid* types,
                        std::size_t types_count,
                        native_binding& native_param,
                        binding& result);

      // Common select interface expected by the generated code.
      //
    public:
      enum result
      {
        success,
        no_data,
        truncated
      };

      void
      execute ();

      void
      cache () const
      {
      }

      std::size_t
      result_size () const
      {
        return row_count_;
      }

      // Load next row columns into bound buffers.
      //
      result
      fetch ()
      {
        return next () ? load () : no_data;
      }

      // Reload truncated columns into bound buffers.
      //
      void
      refetch ()
      {
        reload ();
      }

      // Free the result set.
      //
      void
      free_result ();

      // Finer grained control of PostgreSQL-specific interface that
      // splits fetch() into next() and load().
      //
    public:
      bool
      next ();

      result
      load ();

      void
      reload ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      binding* param_;
      native_binding* native_param_;

      binding& result_;

      auto_handle<PGresult> handle_;
      std::size_t row_count_;
      std::size_t current_row_;
    };

    struct LIBODB_PGSQL_EXPORT auto_result
    {
      explicit auto_result (select_statement& s): s_ (s) {}
      ~auto_result () {s_.free_result ();}

    private:
      auto_result (const auto_result&);
      auto_result& operator= (const auto_result&);

    private:
      select_statement& s_;
    };

    class LIBODB_PGSQL_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        bool process_text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        binding* returning);

      insert_statement (connection_type& conn,
                        const char* name,
                        const char* text,
                        bool process_text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        binding* returning,
                        bool copy_name_text = true);

      // Return true if successful and false if the row is a duplicate.
      // All other errors are reported by throwing exceptions.
      //
      bool
      execute ();

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions&);

      // Return true if successful and false if this row is a duplicate.
      // All other errors are reported via exceptions.
      //
      bool
      result (std::size_t i)
      {
        return param_.status[i] != 0;
      }

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

    private:
      binding& param_;
      native_binding& native_param_;
      binding* returning_;
    };

    class LIBODB_PGSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        bool process_text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param);

      update_statement (connection_type& conn,
                        const char* name,
                        const char* text,
                        bool process_text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        bool copy_name_text = true);

      unsigned long long
      execute ();

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions&);

      // Return the number of rows affected (updated) by the parameter
      // set. All errors are reported by throwing exceptions.
      //
      static const unsigned long long result_unknown = ~0ULL;

      unsigned long long
      result (std::size_t i)
      {
        return param_.status[i];
      }

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);

    private:
      binding& param_;
      native_binding& native_param_;
    };

    class LIBODB_PGSQL_EXPORT delete_statement: public statement
    {
    public:
      virtual
      ~delete_statement ();

      delete_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param);

      delete_statement (connection_type& conn,
                        const char* name,
                        const char* text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        bool copy_name_text = true);

      delete_statement (connection_type& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        native_binding& native_param);

      unsigned long long
      execute ();

      // Return the number of parameter sets (out of n) that were attempted.
      //
      std::size_t
      execute (std::size_t n, multiple_exceptions&);

      // Return the number of rows affected (deleted) by the parameter
      // set. All errors are reported by throwing exceptions.
      //
      static const unsigned long long result_unknown = ~0ULL;

      unsigned long long
      result (std::size_t i)
      {
        return param_->status[i];
      }

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);

    private:
      binding* param_;
      native_binding& native_param_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_STATEMENT_HXX
