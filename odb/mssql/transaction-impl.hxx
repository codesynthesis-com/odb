// file      : odb/mssql/transaction-impl.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_TRANSACTION_IMPL_HXX
#define ODB_MSSQL_TRANSACTION_IMPL_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class LIBODB_MSSQL_EXPORT transaction_impl: public odb::transaction_impl
    {
    public:
      typedef mssql::database database_type;
      typedef mssql::connection connection_type;

      transaction_impl (database_type&);
      transaction_impl (connection_ptr);

      virtual
      ~transaction_impl ();

      virtual void
      start ();

      virtual void
      commit ();

      virtual void
      rollback ();

      connection_type&
      connection ();

    private:
      connection_ptr connection_;
    };
  }
}

#include <odb/mssql/transaction-impl.ixx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_TRANSACTION_IMPL_HXX
