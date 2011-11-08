// file      : odb/oracle/oracle-types.cxx
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
    get (sb2& y, ub1& m, ub1& d, ub1& h, ub1& minute, ub1& s, ub4& ns) const
    {
      assert (descriptor.get () != 0);

      sword r (OCIDateTimeGetDate (environment,
                                   error,
                                   descriptor,
                                   &y,
                                   &m,
                                   &d));

      if (r != OCI_SUCCESS)
        translate_error (error, r);

      r = OCIDateTimeGetTime (environment,
                              error,
                              descriptor,
                              &h,
                              &minute,
                              &s,
                              &ns);

      if (r != OCI_SUCCESS)
        translate_error (error, r);
    }

    void datetime::
    set (sb2 y, ub1 m, ub1 d, ub1 h, ub1 minute, ub1 s, ub4 ns)
    {
      if (descriptor.get () != 0)
      {
        sword r (OCIDateTimeConstruct (environment,
                                       error,
                                       descriptor,
                                       y,
                                       m,
                                       d,
                                       h,
                                       minute,
                                       s,
                                       ns,
                                       0,
                                       0));

        if (r != OCI_SUCCESS)
          translate_error (error, r);
      }
      else
      {
        year_ = y;
        month_ = m;
        day_ = d;
        hour_ = h;
        minute_ = minute;
        second_ = s;
        nanosecond_ = ns;
      }
    }

    void datetime::
    set ()
    {
      assert (descriptor.get () != 0);

      sword r (OCIDateTimeConstruct (environment,
                                     error,
                                     descriptor,
                                     year_,
                                     month_,
                                     day_,
                                     hour_,
                                     minute_,
                                     second_,
                                     nanosecond_,
                                     0,
                                     0));

      if (r != OCI_SUCCESS)
        translate_error (error, r);
    }

    void interval_ym::
    get (sb4& y, sb4& m) const
    {
      assert (descriptor.get () != 0);

      sword r (OCIIntervalGetYearMonth (environment,
                                        error,
                                        &y,
                                        &m,
                                        descriptor));

      if (r != OCI_SUCCESS)
        translate_error (error, r);
    }

    void interval_ym::
    set (sb4 y, sb4 m)
    {
      if (descriptor.get () != 0)
      {
        sword r (OCIIntervalSetYearMonth (environment,
                                          error,
                                          y,
                                          m,
                                          descriptor));

        if (r != OCI_SUCCESS)
          translate_error (error, r);
      }
      else
      {
        year_ = y;
        month_ = m;
      }
    }

    void interval_ym::
    set ()
    {
      assert (descriptor.get () != 0);

      sword r (OCIIntervalSetYearMonth (environment,
                                        error,
                                        year_,
                                        month_,
                                        descriptor));

      if (r != OCI_SUCCESS)
        translate_error (error, r);
    }

    void interval_ds::
    get (sb4& d, sb4& h, sb4& m, sb4& s, sb4& ns) const
    {
      assert (descriptor.get () != 0);

      sword r (OCIIntervalGetDaySecond (environment,
                                        error,
                                        &d,
                                        &h,
                                        &m,
                                        &s,
                                        &ns,
                                        descriptor));

      if (r != OCI_SUCCESS)
        translate_error (error, r);
    }

    void interval_ds::
    set (sb4 d, sb4 h, sb4 m, sb4 s, sb4 ns)
    {
      if (descriptor.get () != 0)
      {
        sword r (OCIIntervalSetDaySecond (environment,
                                          error,
                                          d,
                                          h,
                                          m,
                                          s,
                                          ns,
                                          descriptor));

        if (r != OCI_SUCCESS)
          translate_error (error, r);
      }
      else
      {
        day_ = d;
        hour_ = h;
        minute_ = m;
        second_ = s;
        nanosecond_ = ns;
      }
    }

    void interval_ds::
    set ()
    {
      assert (descriptor.get () != 0);

      sword r (OCIIntervalSetDaySecond (environment,
                                        error,
                                        day_,
                                        hour_,
                                        minute_,
                                        second_,
                                        nanosecond_,
                                        descriptor));

      if (r != OCI_SUCCESS)
        translate_error (error, r);
    }
  }
}
