// file      : odb/oracle/oracle-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <oci.h>

#include <odb/oracle/oracle-types.hxx>
#include <odb/oracle/error.hxx>

namespace odb
{
  namespace oracle
  {
    void datetime::
    get () const
    {
      assert (descriptor.get () != 0);

      sword r (OCIDateTimeGetDate (descriptor.environment,
                                   descriptor.error,
                                   descriptor,
                                   &fields.year,
                                   &fields.month,
                                   &fields.day));

      if (r != OCI_SUCCESS)
        translate_error (descriptor.error, r);

      r = OCIDateTimeGetTime (descriptor.environment,
                              descriptor.error,
                              descriptor,
                              &fields.hour,
                              &fields.minute,
                              &fields.second,
                              &fields.nanosecond);

      if (r != OCI_SUCCESS)
        translate_error (descriptor.error, r);
    }

    void datetime::
    set ()
    {
      if (descriptor.get () != 0)
      {
        sword r (OCIDateTimeConstruct (descriptor.environment,
                                       descriptor.error,
                                       descriptor,
                                       fields.year,
                                       fields.month,
                                       fields.day,
                                       fields.hour,
                                       fields.minute,
                                       fields.second,
                                       fields.nanosecond,
                                       0,
                                       0));

        if (r != OCI_SUCCESS)
          translate_error (descriptor.error, r);
      }
    }

    void interval_ym::
    get () const
    {
      assert (descriptor.get () != 0);

      sword r (OCIIntervalGetYearMonth (descriptor.environment,
                                        descriptor.error,
                                        &fields.year,
                                        &fields.month,
                                        descriptor));

      if (r != OCI_SUCCESS)
        translate_error (descriptor.error, r);
    }

    void interval_ym::
    set ()
    {
      if (descriptor.get () != 0)
      {
        sword r (OCIIntervalSetYearMonth (descriptor.environment,
                                          descriptor.error,
                                          fields.year,
                                          fields.month,
                                          descriptor));

        if (r != OCI_SUCCESS)
          translate_error (descriptor.error, r);
      }
    }

    void interval_ds::
    get () const
    {
      assert (descriptor.get () != 0);

      sword r (OCIIntervalGetDaySecond (descriptor.environment,
                                        descriptor.error,
                                        &fields.day,
                                        &fields.hour,
                                        &fields.minute,
                                        &fields.second,
                                        &fields.nanosecond,
                                        descriptor));

      if (r != OCI_SUCCESS)
        translate_error (descriptor.error, r);
    }

    void interval_ds::
    set ()
    {
      if (descriptor.get () != 0)
      {
        sword r (OCIIntervalSetDaySecond (descriptor.environment,
                                          descriptor.error,
                                          fields.day,
                                          fields.hour,
                                          fields.minute,
                                          fields.second,
                                          fields.nanosecond,
                                          descriptor));

        if (r != OCI_SUCCESS)
          translate_error (descriptor.error, r);
      }
    }
  }
}
