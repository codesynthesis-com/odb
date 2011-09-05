// file      : odb/oracle/transaction-impl.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <oci.h>

#include <odb/oracle/database.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/error.hxx>
#include <odb/oracle/exceptions.hxx>
#include <odb/oracle/transaction-impl.hxx>

namespace odb
{
  namespace oracle
  {
    transaction_impl::
    transaction_impl (database_type& db)
        : odb::transaction_impl (db)
    {
    }

    transaction_impl::
    transaction_impl (connection_ptr c)
        : odb::transaction_impl (c->database (), *c), connection_ (c)
    {
    }

    transaction_impl::
    ~transaction_impl ()
    {
    }

    void transaction_impl::
    start ()
    {
      database_type& db (static_cast<database_type&> (database_));

      if (connection_ == 0)
      {
        connection_ = db.connection ();
        odb::transaction_impl::connection_ = connection_.get ();
      }

      OCISvcCtx* svc (connnection_->handle ());
      OCIError* err (connection_->error_handle ());

      // Allocate a transaction handle if there is none associated with
      // the connection.
      //
      OCITrans* t (0);
      sword s (OCIAttrGet (svc,
                           OCI_HTYPE_SVCCTX,
                           reinterpret_cast<void*> (&t),
                           0,
                           OCI_ATTR_TRANS,
                           err));

      if (s == OCI_ERROR || s == OCI_INVALID_HANDLE)
        translate_error (err, s);
      else if (t == 0)
      {
        auto_handle<OCITrans> auto_t;

        s = OCIHandleAlloc (db.environment (),
                            reinterpret_cast<void**> (&t),
                            OCI_HTYPE_TRANS,
                            0,
                            0);

        if (s != OCI_SUCCESS)
          throw invalid_oci_handle ();

        auto_t.reset (t);

        s = OCIAttrSet (svc,
                        OCI_HTYPE_SVCCTX,
                        reinterpret_cast<void*> (t),
                        0,
                        OCI_ATTR_TRANS,
                        err);

        if (s == OCI_ERROR || s == OCI_INVALID_HANDLE)
          translate_error (err, s);

        auto_t.release ();
      }

      // We never use OCITransDetach so the timeout parameter is
      // of no consequence.
      //
      s = OCITransStart (svc,
                         err,
                         0,
                         OCI_TRANS_NEW);

      if (s == OCI_ERROR || s == OCI_INVALID_HANDLE)
        translate_error (err, s);
    }

    void transaction_impl::
    commit ()
    {
      sword s (OCITransCommit (connection_->handle (),
                               connection_->error_handle (),
                               OCI_DEFAULT));

      if (s == OCI_ERROR || s == OCI_INVALID_HANDLE)
        translate_error (err_, s);
    }

    void transaction_impl::
    rollback ()
    {
      sword s (OCITransRollback (connection_->handle (),
                                 connection_->error_handle (),
                                 OCI_DEFAULT));

      if (s == OCI_ERROR || s == OCI_INVALID_HANDLE)
        translate_error (err_, s);
    }
  }
}
