// file      : odb/transaction.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <utility> // std::move

#include <odb/connection.hxx>

namespace odb
{
  inline transaction::
  transaction ()
      : finalized_ (true),
        free_callback_ (max_callback_count),
        callback_count_ (0)
  {
  }

  inline transaction::
  transaction (std::unique_ptr<transaction_impl> impl, bool make_current)
      : finalized_ (true),
        free_callback_ (max_callback_count),
        callback_count_ (0)
  {
    reset (std::move (impl), make_current);
  }

  inline transaction::database_type& transaction::
  database ()
  {
    return impl_->database ();
  }

  inline transaction::connection_type& transaction::
  connection ()
  {
    return impl_->connection (0);
  }

  inline transaction::connection_type& transaction::
  connection (database_type& db)
  {
    return impl_->connection (&db);
  }

  inline transaction_impl& transaction::
  implementation ()
  {
    return *impl_;
  }

  inline void transaction::
  tracer (tracer_type& t)
  {
    impl_->tracer (&t);
  }

  inline void transaction::
  tracer (tracer_type* t)
  {
    impl_->tracer (t);
  }

  inline transaction::tracer_type* transaction::
  tracer () const
  {
    return impl_->tracer ();
  }
}
