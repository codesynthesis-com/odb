// file      : odb/oracle/oracle-types.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <odb/oracle/oracle-types.hxx>
#include <odb/oracle/exceptions.hxx>
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
    lob (const lob& x)
        : environment (x.environment),
          error (x.error),
          locator (0),
          buffer (x.buffer),
          position (x.position)
    {
      // Watch out for exception safety.
      //
      if (x.locator != 0)
        clone (x);
    }

    lob& lob::
    operator= (const lob& x)
    {
      // Watch out for exception safety.
      //
      if (this != &x)
      {
        if (x.locator != 0)
          clone (x);
        else
        {
          if (locator != 0)
          {
            OCIDescriptorFree (locator, OCI_DTYPE_LOB);
            locator = 0;
          }
        }

        environment = x.environment;
        error = x.error;
        buffer = x.buffer;
        position = x.position;
      }

      return *this;
    }

    void lob::
    clone (const lob& x)
    {
      // Watch out for exception safety.
      //
      sword r;
      bool alloc (locator == 0);

      if (alloc)
      {
        void* d (0);
        r = OCIDescriptorAlloc (x.environment, &d, OCI_DTYPE_LOB, 0, 0);

        if (r != OCI_SUCCESS)
          throw invalid_oci_handle ();

        locator = static_cast<OCILobLocator*> (d);
      }

      r = OCILobAssign (x.environment, x.error, x.locator, &locator);

      if (r != OCI_SUCCESS)
      {
        if (alloc)
        {
          OCIDescriptorFree (locator, OCI_DTYPE_LOB);
          locator = 0;
        }

        translate_error (x.error, r);
      }
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
    datetime (const datetime& x)
        : descriptor (0), flags (x.flags)
    {
      x.get (year_, month_, day_, hour_, minute_, second_, nanosecond_);
    }

    datetime& datetime::
    operator= (const datetime& x)
    {
      if (this != &x)
      {
        if (descriptor != 0 && (flags & descriptor_free))
        {
          OCIDescriptorFree (descriptor, OCI_DTYPE_TIMESTAMP);
          descriptor = 0;
        }

        flags = x.flags;
        x.get (year_, month_, day_, hour_, minute_, second_, nanosecond_);
      }

      return *this;
    }

    void datetime::
    get (sb2& y, ub1& m, ub1& d, ub1& h, ub1& mi, ub1& s, ub4& ns) const
    {
      if (descriptor != 0)
      {
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
                                &mi,
                                &s,
                                &ns);

        if (r != OCI_SUCCESS)
          translate_error (error, r);
      }
      else
      {
        y = year_;
        m = month_;
        d = day_;
        h = hour_;
        mi = minute_;
        s = second_;
        ns = nanosecond_;
      }
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
    interval_ym (const interval_ym& x)
        : descriptor (0), flags (x.flags)
    {
      x.get (year_, month_);
    }

    interval_ym& interval_ym::
    operator= (const interval_ym& x)
    {
      if (this != &x)
      {
        if (descriptor != 0 && (flags & descriptor_free))
        {
          OCIDescriptorFree (descriptor, OCI_DTYPE_INTERVAL_YM);
          descriptor = 0;
        }

        flags = x.flags;
        x.get (year_, month_);
      }

      return *this;
    }

    void interval_ym::
    get (sb4& y, sb4& m) const
    {
      if (descriptor != 0)
      {
        sword r (OCIIntervalGetYearMonth (environment,
                                          error,
                                          &y,
                                          &m,
                                          descriptor));

        if (r != OCI_SUCCESS)
          translate_error (error, r);
      }
      else
      {
        y = year_;
        m = month_;
      }
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
    interval_ds (const interval_ds& x)
        : descriptor (0), flags (x.flags)
    {
      x.get (day_, hour_, minute_, second_, nanosecond_);
    }

    interval_ds& interval_ds::
    operator= (const interval_ds& x)
    {
      if (this != &x)
      {
        if (descriptor != 0 && (flags & descriptor_free))
        {
          OCIDescriptorFree (descriptor, OCI_DTYPE_TIMESTAMP);
          descriptor = 0;
        }

        flags = x.flags;
        x.get (day_, hour_, minute_, second_, nanosecond_);
      }

      return *this;
    }

    void interval_ds::
    get (sb4& d, sb4& h, sb4& m, sb4& s, sb4& ns) const
    {
      if (descriptor != 0)
      {
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
      else
      {
        d = day_;
        h = hour_;
        m = minute_;
        s = second_;
        ns = nanosecond_;
      }
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
