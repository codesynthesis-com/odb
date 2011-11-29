// file      : odb/oracle/error.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <odb/details/buffer.hxx>

#include <odb/oracle/error.hxx>
#include <odb/oracle/exceptions.hxx>
#include <odb/oracle/connection.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    void
    translate_error (void* h, ub4 t, sword s, connection* conn)
    {
      assert (s != OCI_SUCCESS && s != OCI_SUCCESS_WITH_INFO);

      if (s != OCI_ERROR)
      {
        switch (s)
        {
        case OCI_STILL_EXECUTING:
          {
            throw database_exception (0, "statement still executing");
            break;
          }
        case OCI_NEED_DATA:
        case OCI_NO_DATA:
          {
            throw database_exception (0, "unhandled OCI_*_DATA condition");
            break;
          }
        case OCI_INVALID_HANDLE:
          {
            throw invalid_oci_handle ();
            break;
          }
        }
      }

      sword r (0);
      sb4 e;
      char b[512];  // Error message will be truncated if it does not fit.

      if (t == OCI_HTYPE_ERROR)
      {
        // Mark the connection as failed if necessary.
        //
        if (conn != 0)
        {
          OCIServer* server (0);
          r = OCIAttrGet (conn->handle (),
                          OCI_HTYPE_SVCCTX,
                          &server,
                          0,
                          OCI_ATTR_SERVER,
                          conn->error_handle ());

          if (r != OCI_SUCCESS)
            throw invalid_oci_handle ();

          ub4 server_status (0);
          r = OCIAttrGet (server,
                          OCI_HTYPE_SERVER,
                          &server_status,
                          0,
                          OCI_ATTR_SERVER_STATUS,
                          conn->error_handle ());

          if (r != OCI_SUCCESS)
            throw invalid_oci_handle ();

          if (server_status == OCI_SERVER_NOT_CONNECTED)
            conn->mark_failed ();
        }

        // We need to translate certain Oracle error codes to special
        // exceptions, such as deadlock, timeout, etc. The problem is we can
        // have multiple records potentially with different error codes. If we
        // have both, say, a deadlock code and some other code, then we should
        // probably throw database_exception, which is more severe. To
        // implement this we are going to pre-scan the records looking for the
        // codes we are interested in. If in the process we see any other code,
        // then we stop and go ahead to prepare and throw database_exception.
        //
        enum code
        {
          code_none,
          code_deadlock,
          code_timeout,
          code_connection_lost,
        };

        code c (code_none);

        for (sb4 i (1);; ++i)
        {
          r = OCIErrorGet (h, i, 0, &e, reinterpret_cast<OraText*> (b), 512, t);

          if (r == OCI_NO_DATA)
            break;

          code nc;

          if (e == 60 || // Deadlock detected while waiting for resource.
              e == 104)  // Deadlock detected; all public servers blocked.
            nc = code_deadlock;
          else if (e == 51 || // Timeout occurred while waiting for a resource.
                   e == 54)   // Resource busy and acquisition timeout expired.
            nc = code_timeout;
          else if (e == 28 ||   // Session has been killed.
                   e == 3135 || // Connection lost contact.
                   e == 3136 || // Inbound connection timed out.
                   e == 3138)   // Connection terminated.
            nc = code_connection_lost;
          else
          {
            c = code_none;
            break;
          }

          if (c != code_none && c != nc)
          {
            // Several different codes.
            //
            c = code_none;
            break;
          }

          c = nc;
        }

        switch (c)
        {
        case code_deadlock:
          throw deadlock ();
        case code_timeout:
          throw timeout ();
        case code_connection_lost:
          throw connection_lost ();
        case code_none:
          break;
        }
      }

      // Some other error code. Prepare database_exception.
      //
      database_exception dbe;

      for (sb4 i (1);; ++i)
      {
        r = OCIErrorGet (h,
                         i,
                         0,
                         &e,
                         reinterpret_cast<OraText*> (b),
                         512,
                         t);

        if (r == OCI_NO_DATA)
          break;

        dbe.append (e, b);
      }

      throw dbe;
    }
  }
}
