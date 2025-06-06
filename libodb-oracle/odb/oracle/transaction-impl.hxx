// file      : odb/oracle/transaction-impl.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_TRANSACTION_IMPL_HXX
#define ODB_ORACLE_TRANSACTION_IMPL_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/oracle-fwd.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class LIBODB_ORACLE_EXPORT transaction_impl: public odb::transaction_impl
    {
    public:
      typedef oracle::database database_type;
      typedef oracle::connection connection_type;

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

#endif // ODB_ORACLE_TRANSACTION_IMPL_HXX
