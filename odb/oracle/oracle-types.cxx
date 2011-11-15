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
    //
    // lob
    //

    lob::
    ~lob ()
    {
      if (locator != 0)
        OCIDescriptorFree (locator, OCI_DTYPE_LOB);
    }

    lob::
    lob (lob& x)
        : locator (x.locator),
          buffer (x.buffer),
          position_context (x.position_context)
    {
      x.locator = 0;
    }

    lob& lob::
    operator= (lob& x)
    {
      if (this != &x)
      {
        if (locator != 0)
          OCIDescriptorFree (locator, OCI_DTYPE_LOB);

        locator = x.locator;
        buffer = x.buffer;
        position_context = x.position_context;

        x.locator = 0;
      }

      return *this;
    }

    //
    // datetime
    //

    datetime::
    ~datetime ()
    {
      if (descriptor != 0 && (flags & descriptor_free))
        OCIDescriptorFree (descriptor, OCI_DTYPE_TIMESTAMP);
    }

    datetime::
    datetime (datetime& x)
        : environment (x.environment),
          error (x.error),
          descriptor (x.descriptor),
          flags (x.flags),
          year_ (x.year_),
          month_ (x.month_),
          day_ (x.day_),
          hour_ (x.hour_),
          minute_ (x.minute_),
          second_ (x.second_),
          nanosecond_ (x.nanosecond_)
      {
        x.descriptor = 0;
      }

    datetime& datetime::
    operator= (datetime& x)
    {
      if (this != &x)
      {
        if (descriptor != 0 && (flags & descriptor_free))
          OCIDescriptorFree (descriptor, OCI_DTYPE_TIMESTAMP);

        environment = x.environment;
        error = x.error;
        descriptor = x.descriptor;
        flags = x.flags;
        year_ = x.year_;
        month_ = x.month_;
        day_ = x.day_;
        hour_ = x.hour_;
        minute_ = x.minute_;
        second_ = x.second_;
        nanosecond_ = x.nanosecond_;

        x.descriptor = 0;
      }

      return *this;
    }

    void datetime::
    get (sb2& y, ub1& m, ub1& d, ub1& h, ub1& minute, ub1& s, ub4& ns) const
    {
      assert (descriptor != 0);

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
      if (descriptor != 0)
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

    //
    // interval_ym
    //

    interval_ym::
    ~interval_ym ()
    {
      if (descriptor != 0 && (flags & descriptor_free))
        OCIDescriptorFree (descriptor, OCI_DTYPE_INTERVAL_YM);
    }

    interval_ym::
    interval_ym (interval_ym& x)
        : environment (x.environment),
          error (x.error),
          descriptor (x.descriptor),
          flags (x.flags),
          year_ (x.year_),
          month_ (x.month_)
      {
        x.descriptor = 0;
      }

    interval_ym& interval_ym::
    operator= (interval_ym& x)
    {
      if (this != &x)
      {
        if (descriptor != 0 && (flags & descriptor_free))
          OCIDescriptorFree (descriptor, OCI_DTYPE_INTERVAL_YM);

        environment = x.environment;
        error = x.error;
        descriptor = x.descriptor;
        flags = x.flags;
        year_ = x.year_;
        month_ = x.month_;

        x.descriptor = 0;
      }

      return *this;
    }

    void interval_ym::
    get (sb4& y, sb4& m) const
    {
      assert (descriptor != 0);

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
      if (descriptor != 0)
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

    //
    // interval_ds
    //

    interval_ds::
    ~interval_ds ()
    {
      if (descriptor != 0 && (flags & descriptor_free))
        OCIDescriptorFree (descriptor, OCI_DTYPE_INTERVAL_DS);
    }

    interval_ds::
    interval_ds (interval_ds& x)
        : environment (x.environment),
          error (x.error),
          descriptor (x.descriptor),
          flags (x.flags),
          day_ (x.day_),
          hour_ (x.hour_),
          minute_ (x.minute_),
          second_ (x.second_),
          nanosecond_ (x.nanosecond_)
      {
        x.descriptor = 0;
      }

    interval_ds& interval_ds::
    operator= (interval_ds& x)
    {
      if (this != &x)
      {
        if (descriptor != 0 && (flags & descriptor_free))
          OCIDescriptorFree (descriptor, OCI_DTYPE_INTERVAL_DS);

        environment = x.environment;
        error = x.error;
        descriptor = x.descriptor;
        flags = x.flags;
        day_ = x.day_;
        hour_ = x.hour_;
        minute_ = x.minute_;
        second_ = x.second_;
        nanosecond_ = x.nanosecond_;

        x.descriptor = 0;
      }

      return *this;
    }

    void interval_ds::
    get (sb4& d, sb4& h, sb4& m, sb4& s, sb4& ns) const
    {
      assert (descriptor != 0);

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
      if (descriptor != 0)
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
  }
}
