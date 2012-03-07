// file      : odb/mssql/statement.hxx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_STATEMENT_HXX
#define ODB_MSSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/forward.hxx>
#include <odb/statement.hxx>

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>
#include <odb/mssql/binding.hxx>
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
      virtual
      ~statement () = 0;

      SQLHSTMT
      handle () const
      {
        return stmt_;
      }

      virtual const char*
      text () const;

    protected:
      statement (connection&, const std::string& text);
      statement (connection&, const char* text, bool copy_text);

    private:
      void
      init (std::size_t text_size);

    protected:
      void
      bind_param (bind*, std::size_t count);

      std::size_t
      bind_result (bind*, std::size_t count);

      SQLRETURN
      execute ();

      // The old_base and new_base arguments can be used to "re-base"
      // the long_callback struct pointer (stored in bind::buffer).
      // This is used by the query machinery to cause stream_result()
      // to use the callback information from a copy of the image
      // instead of the bound image.
      //
      void
      stream_result (bind*,
                     std::size_t start,
                     std::size_t count,
                     void* old_base = 0,
                     void* new_base = 0);

    protected:
      connection& conn_;
      std::string text_copy_;
      const char* text_;
      auto_handle<SQL_HANDLE_STMT> stmt_;
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
      select_statement (connection& conn,
                        const std::string& text,
                        binding& param,
                        binding& result);

      select_statement (connection& conn,
                        const char* text,
                        binding& param,
                        binding& result,
                        bool copy_text = true);

      select_statement (connection& conn,
                        const std::string& text,
                        binding& result);

      select_statement (connection& conn,
                        const char* text,
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
        bool ld (first_long_ != result_.count);

        if (ld)
          statement::stream_result (result_.bind,
                                    first_long_,
                                    result_.count,
                                    old_base,
                                    new_base);
        return ld;
      }

      void
      free_result ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      binding& result_;
      std::size_t first_long_; // First long data column.
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

    class LIBODB_MSSQL_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection& conn,
                        const std::string& text,
                        binding& param,
                        bool returning);

      insert_statement (connection& conn,
                        const char* text,
                        binding& param,
                        bool returning,
                        bool copy_text = true);

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
      void
      init_result ();

    private:
      bool returning_;
      bool batch_;
      unsigned long long id_;
      SQLLEN id_size_ind_;
    };

    class LIBODB_MSSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection& conn,
                        const std::string& text,
                        binding& param);

      update_statement (connection& conn,
                        const char* text,
                        binding& param,
                        bool copy_text = true);

      unsigned long long
      execute ();

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);
    };

    class LIBODB_MSSQL_EXPORT delete_statement: public statement
    {
    public:
      virtual
      ~delete_statement ();

      delete_statement (connection& conn,
                        const std::string& text,
                        binding& param);

      delete_statement (connection& conn,
                        const char* text,
                        binding& param,
                        bool copy_text = true);

      unsigned long long
      execute ();

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_STATEMENT_HXX
