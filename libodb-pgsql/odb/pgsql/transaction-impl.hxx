// file      : odb/pgsql/transaction-impl.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_TRANSACTION_IMPL_HXX
#define ODB_PGSQL_TRANSACTION_IMPL_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class LIBODB_PGSQL_EXPORT transaction_impl: public odb::transaction_impl
    {
    public:
      typedef pgsql::database database_type;
      typedef pgsql::connection connection_type;

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

    private:
      connection_ptr connection_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_TRANSACTION_IMPL_HXX
