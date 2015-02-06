// file      : odb/oracle/auto-handle.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <odb/oracle/auto-handle.hxx>

namespace odb
{
  namespace oracle
  {
    void
    oci_handle_free (void* h, ub4 t)
    {
      OCIHandleFree (h, t);
    }

    void handle_traits<OCISvcCtx>::
    release (OCISvcCtx* h, OCIError* e)
    {
      OCISessionRelease (h, e, 0, 0, OCI_DEFAULT);
    }

    void handle_traits<OCIStmt>::
    release (OCIStmt* h, ub4 m, OCIError* e)
    {
      OCIStmtRelease (h, e, 0, 0, m);
    }

    const ub4 handle_type_traits<OCIEnv>::htype = OCI_HTYPE_ENV;
    const ub4 handle_type_traits<OCIError>::htype = OCI_HTYPE_ERROR;
    const ub4 handle_type_traits<OCISvcCtx>::htype = OCI_HTYPE_SVCCTX;
    const ub4 handle_type_traits<OCIStmt>::htype = OCI_HTYPE_STMT;
    const ub4 handle_type_traits<OCIAuthInfo>::htype = OCI_HTYPE_AUTHINFO;
    const ub4 handle_type_traits<OCITrans>::htype = OCI_HTYPE_TRANS;
  }
}
