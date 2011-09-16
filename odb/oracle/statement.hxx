// file      : odb/oracle/statement.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_STATEMENT_HXX
#define ODB_ORACLE_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/forward.hxx>

#include <odb/details/shared-ptr.hxx>

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
    class LIBODB_ORACLE_EXPORT statement: public details::shared_base
    {
    public:
      virtual
      ~statement () = 0;

    protected:
      statement (connection&, const std::string& statement);

      // Bind output parameters to this statement. This function must only
      // be called once. Multiple calls to it will result in memory leaks due
      // to lost OCIBind resources.
      //
      void
      bind_param (bind*, std::size_t count, std::size_t offset);

      // Bind input parameters to this statement. This function must only be
      // called once. Multiple calls to it will result in memory leaks due to
      // lost OCIDefine resources.
      //
      void
      bind_result (bind*,
                   std::size_t count,
                   std::size_t lob_prefetch_size = 0);

      // Stream the result LOBs, calling user callbacks where necessary.
      //
      void
      stream_result_lobs ();

    protected:
      connection& conn_;
      auto_handle<OCIStmt> stmt_;

      bind* result_binds_;
      std::size_t count_;
    };

    class LIBODB_ORACLE_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      select_statement (connection& conn,
                        const std::string& statement,
                        binding& cond,
                        binding& data,
                        std::size_t lob_prefetch_size = 0);

      select_statement (connection& conn,
                        const std::string& statement,
                        binding& data,
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
      free_result ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      bool done_;
    };

    class LIBODB_ORACLE_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection& conn,
                        const std::string& statement,
                        binding& data,
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
      // Otherwise, we revert to using a 32-bit unsigned integer.
      //
    public:
      struct id_bind_type
      {
        union
        {
          unsigned int value32;
          unsigned long long value64;
        } id;

        sb2 indicator;
      };

    private:
      id_bind_type id_bind_;
    };

    class LIBODB_ORACLE_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection& conn,
                        const std::string& statement,
                        binding& cond,
                        binding& data);
      void
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
                        const std::string& statement,
                        binding& cond);

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
