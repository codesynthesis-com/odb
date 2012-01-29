// file      : odb/oracle/statement.hxx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_STATEMENT_HXX
#define ODB_ORACLE_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/forward.hxx>
#include <odb/statement.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/binding.hxx>
#include <odb/oracle/forward.hxx>
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
      virtual
      ~statement () = 0;

      OCIStmt*
      handle () const
      {
        return stmt_;
      }

      virtual const char*
      text () const;

    protected:
      statement (connection&, const std::string& text);
      statement (connection&, const char* text);

    private:
      void
      init (const char* text, std::size_t text_size);

    protected:
      struct unbind
      {
        oracle::bind* bind;  // Corresponding bind entry.
        void* value;         // Actual value passed to OCIBindByPos.
      };

      // Bind parameters for this statement. This function must only
      // be called once. Multiple calls to it will result in memory
      // leaks due to lost OCIBind resources.
      //
      void
      bind_param (bind*, std::size_t count);

      // Bind results for this statement. This function must only be
      // called once. Multiple calls to it will result in memory leaks
      // due to lost OCIDefine resources.
      //
      void
      bind_result (bind*,
                   std::size_t count,
                   std::size_t lob_prefetch_size = 0);

      // Rebind LOB input parameters. If a query has made a private copy of
      // the shared image, any LOB handles that were previously owned by the
      // shared image are now owned by the private image of the query. These
      // LOB handles need to be reallocated and redefined so that any unfetched
      // results may be fetched.
      //
      void
      rebind_result (bind*,
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
      connection& conn_;
      auto_handle<OCIStmt> stmt_;

      unbind* udata_;
      std::size_t usize_;
    };

    class LIBODB_ORACLE_EXPORT generic_statement: public statement
    {
    public:
      virtual
      ~generic_statement ();

      generic_statement (connection&, const std::string& text);
      generic_statement (connection&, const char* text);

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

      select_statement (connection& conn,
                        const std::string& text,
                        binding& param,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection& conn,
                        const char* text,
                        binding& param,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection& conn,
                        const std::string& text,
                        binding& result,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection& conn,
                        const char* text,
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
      std::size_t result_version_;
      const std::size_t lob_prefetch_size_;
      bool done_;
    };

    class LIBODB_ORACLE_EXPORT insert_statement: public statement
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
                        bool returning);

      // Return true if successful and false if the row is a duplicate. All
      // other errors are reported by throwing exceptions.
      //
      bool
      execute ();

      unsigned long long
      id ();

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

      // Only OCI versions 11.2 and greater support conversion of the internal
      // Oracle type NUMBER to an external 64-bit integer type. If we detect
      // version 11.2 or greater we provide an unsigned long long image.
      // Otherwise, we revert to using a NUMBER image and manipulate it using
      // the custom conversion algorithms found in details/number.hxx.
      //
    public:
      struct id_bind_type
      {
        union
        {
          struct
          {
            char buffer[21];
            ub4 size;
          } number;

          unsigned long long integer;
        } id;

        sb2 indicator;
      };

    private:
      void
      init (binding& param, bool returning);

    private:
      id_bind_type id_bind_;
    };

    class LIBODB_ORACLE_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection& conn,
                        const std::string& text,
                        binding& param);

      update_statement (connection& conn, const char* text, binding& param);

      unsigned long long
      execute ();

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);
    };

    class LIBODB_ORACLE_EXPORT delete_statement: public statement
    {
    public:
      virtual
      ~delete_statement ();

      delete_statement (connection& conn,
                        const std::string& text,
                        binding& param);

      delete_statement (connection& conn, const char* text, binding& param);

      unsigned long long
      execute ();

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_STATEMENT_HXX
