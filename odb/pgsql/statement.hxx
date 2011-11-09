// file      : odb/pgsql/statement.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_STATEMENT_HXX
#define ODB_PGSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/forward.hxx>
#include <odb/statement.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/binding.hxx>
#include <odb/pgsql/pgsql-fwd.hxx> // PGresult
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
      virtual
      ~statement () = 0;

      const char*
      name () const
      {
        return name_;
      }

      virtual const char*
      text () const;

      void
      deallocate ();

      // Adapt an ODB binding to a native PostgreSQL parameter binding.
      //
      static void
      bind_param (native_binding&, const binding&);

      // Populate an ODB binding given a PostgreSQL result. If the truncated
      // argument is true, then only truncated columns are extracted. Return
      // true if all the data was extracted successfully and false if one or
      // more columns were truncated.
      //
      static bool
      bind_result (bind*,
                   std::size_t count,
                   PGresult*,
                   std::size_t row,
                   bool truncated = false);

    protected:
      statement (connection&,
                 const std::string& name,
                 const std::string& text,
                 const Oid* types,
                 std::size_t types_count);

      statement (connection&,
                 const char* name,
                 const char* text,
                 bool copy_name_text,
                 const Oid* types,
                 std::size_t types_count);

    private:
      void
      init (const Oid* types, std::size_t types_count);

    protected:
      connection& conn_;

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

      select_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        binding& result);

      select_statement (connection& conn,
                        const char* name,
                        const char* stmt,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        binding& result,
                        bool copy_name_text = true);

      select_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
                        binding& result);

      select_statement (connection& conn,
                        const char* name,
                        const char* text,
                        binding& result,
                        bool copy_name_text = true);

      select_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
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

    class LIBODB_PGSQL_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        bool returning);

      insert_statement (connection& conn,
                        const char* name,
                        const char* text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        bool returning,
                        bool copy_name_text = true);

      // Return true if successful and false if the row is a duplicate.
      // All other errors are reported by throwing exceptions.
      //
      bool
      execute ();

      unsigned long long
      id ()
      {
        return id_;
      }

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

    private:
      binding& param_;
      native_binding& native_param_;

      bool returning_;
      unsigned long long id_;
    };

    class LIBODB_PGSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param);

      update_statement (connection& conn,
                        const char* name,
                        const char* text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        bool copy_name_text = true);

      unsigned long long
      execute ();

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

      delete_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param);

      delete_statement (connection& conn,
                        const char* name,
                        const char* text,
                        const Oid* types,
                        std::size_t types_count,
                        binding& param,
                        native_binding& native_param,
                        bool copy_name_text = true);

      delete_statement (connection& conn,
                        const std::string& name,
                        const std::string& text,
                        const Oid* types,
                        std::size_t types_count,
                        native_binding& native_param);

      unsigned long long
      execute ();

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
