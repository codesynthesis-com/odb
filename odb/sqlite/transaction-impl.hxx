// file      : odb/sqlite/transaction-impl.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_TRANSACTION_IMPL_HXX
#define ODB_SQLITE_TRANSACTION_IMPL_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>
#include <odb/details/shared-ptr.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    class LIBODB_SQLITE_EXPORT transaction_impl: public odb::transaction_impl
    {
    protected:
      friend class database;
      friend class transaction;

      typedef sqlite::database database_type;
      typedef sqlite::connection connection_type;

      enum lock
      {
        deferred,
        immediate,
        exclusive
      };

      transaction_impl (database_type&, lock);

      virtual
      ~transaction_impl ();

      virtual void
      commit ();

      virtual void
      rollback ();

      connection_type&
      connection ();

    private:
      details::shared_ptr<connection_type> connection_;
    };
  }
}

#include <odb/sqlite/transaction-impl.ixx>

#include <odb/post.hxx>

#endif // ODB_SQLITE_TRANSACTION_IMPL_HXX
