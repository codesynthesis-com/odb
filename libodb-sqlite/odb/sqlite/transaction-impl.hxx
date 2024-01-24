// file      : odb/sqlite/transaction-impl.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_TRANSACTION_IMPL_HXX
#define ODB_SQLITE_TRANSACTION_IMPL_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>

#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    class LIBODB_SQLITE_EXPORT transaction_impl: public odb::transaction_impl
    {
    public:
      typedef sqlite::database database_type;
      typedef sqlite::connection connection_type;

      enum lock
      {
        deferred,
        immediate,
        exclusive
      };

      transaction_impl (database_type&, lock);
      transaction_impl (connection_ptr, lock);

      virtual
      ~transaction_impl ();

      virtual void
      start ();

      virtual void
      commit ();

      virtual void
      rollback ();

      virtual odb::connection&
      connection (odb::database*);

      virtual void
      tracer (odb::tracer*);

      virtual odb::tracer*
      tracer () const;

    private:
      connection_ptr connection_;
      lock lock_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_TRANSACTION_IMPL_HXX
