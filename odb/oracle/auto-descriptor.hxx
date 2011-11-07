// file      : odb/oracle/auto-descriptor.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_AUTO_DESCRIPTOR_HXX
#define ODB_ORACLE_AUTO_DESCRIPTOR_HXX

#include <odb/pre.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-fwd.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    enum descriptor_type
    {
      dt_default = 0,
      dt_param,
      dt_lob,
      dt_timestamp,
      dt_interval_ym,
      dt_interval_ds
    };

    LIBODB_ORACLE_EXPORT void
    oci_descriptor_free (void* descriptor, descriptor_type type);

    //
    // descriptor_type_traits
    //

    template <typename D>
    struct default_descriptor_type_traits;

    template <>
    struct default_descriptor_type_traits<OCIParam>
    { static const descriptor_type dtype = dt_param; };

    template <>
    struct default_descriptor_type_traits<OCILobLocator>
    { static const descriptor_type dtype = dt_lob; };

    //
    // auto_descriptor_base
    //

    template <typename D, descriptor_type Type>
    struct auto_descriptor_base
    {
      static void
      release (D* d)
      {
        oci_descriptor_free (d, Type);
      }
    };

    template <typename D>
    struct auto_descriptor_base<D, dt_default>
    {
      static void
      release (D* d)
      {
        oci_descriptor_free (d, default_descriptor_type_traits<D>::dtype);
      }
    };

    //
    // auto_descriptor
    //

    template <typename D, descriptor_type T = dt_default>
    class auto_descriptor: auto_descriptor_base<D, T>
    {
    public:
      auto_descriptor (D* d = 0)
          : d_ (d)
      {
      }

      ~auto_descriptor ()
      {
        if (d_ != 0)
          release (d_);
      }

      operator D* () const
      {
        return d_;
      }

      D*&
      get ()
      {
        return d_;
      }

      D*
      get () const
      {
        return d_;
      }

      void
      reset (D* d = 0)
      {
        if (d_ != 0)
          release (d_);

        d_ = d;
      }

    private:
      auto_descriptor (const auto_descriptor&);
      auto_descriptor& operator= (const auto_descriptor&);

    protected:
      D* d_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_AUTO_DESCRIPTOR_HXX
