// file      : odb/oracle/auto-descriptor.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <odb/oracle/auto-descriptor.hxx>

namespace odb
{
  namespace oracle
  {
    static const ub4 oci_descriptor_types[] =
    {
      OCI_DTYPE_PARAM,
      OCI_DTYPE_LOB,
      OCI_DTYPE_TIMESTAMP,
      OCI_DTYPE_INTERVAL_YM,
      OCI_DTYPE_INTERVAL_DS
    };

    void
    oci_descriptor_free (void* d, descriptor_type type)
    {
      OCIDescriptorFree (d, oci_descriptor_types[type]);
    }
  }
}
