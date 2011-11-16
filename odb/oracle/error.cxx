// file      : odb/oracle/error.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <odb/details/buffer.hxx>

#include <odb/oracle/error.hxx>
#include <odb/oracle/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    void
    translate_error (void* h, ub4 t, sword s)
    {
      assert (s == OCI_ERROR || s == OCI_INVALID_HANDLE);

      if (s == OCI_INVALID_HANDLE)
        throw invalid_oci_handle ();

      sb4 e;
      details::buffer b;
      b.capacity (128);

      bool trunc (true);
      while (trunc)
      {
        trunc = OCIErrorGet (h,
                             1,
                             0,
                             &e,
                             reinterpret_cast<OraText*> (b.data ()),
                             b.capacity (),
                             t) == OCI_ERROR;

        if (trunc)
          b.capacity (b.capacity () * 2);
      }

      // @@ Need to find a source of OCI specific codes.
      //
      // There are no symbolic definitions for error codes in the OCI
      // header files.
      //
      switch (e)
      {
      case 60:
        throw deadlock ();
      case 3135:
      case 3136:
        throw connection_lost ();
      default:
        throw database_exception (e, b.data ());
      }
    }
  }
}
