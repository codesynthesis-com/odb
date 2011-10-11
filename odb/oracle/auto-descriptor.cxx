// file      : odb/oracle/auto-descriptor.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <odb/oracle/auto-descriptor.hxx>

namespace odb
{
  namespace oracle
  {
    void
    oci_descriptor_free (void* d, ub4 type)
    {
      OCIDescriptorFree (d, type);
    }

    const ub4 descriptor_type_traits<OCIParam>::dtype = OCI_DTYPE_PARAM;
    const ub4 descriptor_type_traits<OCILobLocator>::dtype = OCI_DTYPE_LOB;
  }
}
