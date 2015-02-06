// file      : odb/oracle/error.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <cstring> // std::strlen
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
    static void
    translate_error (void* h, ub4 htype, sword r, connection* conn,
                     size_t pos, multiple_exceptions* mex)
    {
      assert (r != OCI_SUCCESS && r != OCI_SUCCESS_WITH_INFO);

      switch (r)
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
      default:
        {
          break;
        }
      }

      sb4 e;
      char b[512];  // Error message will be truncated if it does not fit.

      if (htype == OCI_HTYPE_ERROR)
      {
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
          code_connection_lost
        };

        code c (code_none);

        for (sb4 i (1);; ++i)
        {
          r = OCIErrorGet (h,
                           i,
                           0,
                           &e,
                           reinterpret_cast<OraText*> (b),
                           512,
                           htype);

          if (r == OCI_NO_DATA)
            break;

          code nc;

          if (e == 60 || // Deadlock detected while waiting for resource.
              e == 104)  // Deadlock detected; all public servers blocked.
            nc = code_deadlock;
          else if (e == 51 || // Timeout occurred while waiting for a resource.
                   e == 54 || // Resource busy and acquisition timeout expired.
                   e == 2049) // Distributed lock timeout.
            nc = code_timeout;
          else if (e == 28 ||   // Session has been killed.
                   e == 3113 || // End-of-file on communication channel.
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

        // Check if the connection is lost. If code is connection_lost,
        // then we know it is gone. If code is deadlock, then the
        // connection is most likely ok.
        //
        if (conn != 0 && (c == code_none || c == code_timeout))
        {
          OCIServer* server;
          r = OCIAttrGet (conn->handle (),
                          OCI_HTYPE_SVCCTX,
                          &server,
                          0,
                          OCI_ATTR_SERVER,
                          conn->error_handle ());

          if (r != OCI_SUCCESS)
            throw invalid_oci_handle ();

          ub4 server_status;
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

        switch (c)
        {
        case code_deadlock:
          throw deadlock ();
        case code_timeout:
          throw timeout ();
        case code_connection_lost:
          {
            if (conn != 0)
              conn->mark_failed ();

            throw connection_lost ();
          }
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
                         htype);

        if (r == OCI_NO_DATA)
          break;

        // Get rid of a trailing newline if there is one.
        //
        size_t n (strlen (b));
        if (n != 0 && b[n - 1] == '\n')
          b[n - 1] = '\0';

        dbe.append (e, b);
      }

      if (mex == 0)
        throw dbe;
      else
        // It could be that some of these errors are fatal. I guess we
        // will just have to learn from experience which ones are. The
        // client code can always treat specific error codes as fatal.
        //
        mex->insert (pos, dbe);
    }

    void
    translate_error (OCIError* h, sword r, connection* c,
                     size_t pos, multiple_exceptions* mex)
    {
      translate_error (h, OCI_HTYPE_ERROR, r, c, pos, mex);
    }

    void
    translate_error (connection& c, sword r)
    {
      translate_error (c.error_handle (), OCI_HTYPE_ERROR, r, &c, 0, 0);
    }

    void
    translate_error (OCIEnv* h)
    {
      translate_error (h, OCI_HTYPE_ENV, OCI_ERROR, 0, 0, 0);
    }
  }
}
