// file      : odb/mysql/transaction-impl.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_TRANSACTION_IMPL_HXX
#define ODB_MYSQL_TRANSACTION_IMPL_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class LIBODB_MYSQL_EXPORT transaction_impl: public odb::transaction_impl
    {
    public:
      typedef mysql::database database_type;
      typedef mysql::connection connection_type;

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

#endif // ODB_MYSQL_TRANSACTION_IMPL_HXX
