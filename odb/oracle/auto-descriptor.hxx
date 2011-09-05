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
    //
    // descriptor_type_traits
    //

    template <typename D>
    struct descriptor_type_traits;

    template <>
    struct descriptor_type_traits<OCIParam>
    { static const ub4 dtype; };

    //
    // descriptor_traits
    //

    LIBODB_ORACLE_EXPORT void
    oci_descriptor_free (void* descriptor, ub4 type);

    template <typename D>
    struct descriptor_traits
    {
      static void
      release (OCIParam* d)
      {
        oci_descriptor_free (d, descriptor_type_traits<D>::dtype);
      }
    };

    template <typename D>
    class auto_descriptor
    {
    public:
      auto_descriptor (D* d = 0)
          : d_ (d)
      {
      }

      ~auto_descriptor ()
      {
        if (d_ != 0)
          descriptor_traits<D>::release (d_);
      }

      operator D* ()
      {
        return d_;
      }

      D*
      get ()
      {
        return d_;
      }

      void
      reset (D* d = 0)
      {
        if (d_ != 0)
          descriptor_traits<D>::release (d_);

        d_ = d;
      }

    private:
      auto_descriptor (const auto_descriptor&);
      auto_descriptor& operator= (const auto_descriptor&);

    private:
      D* d_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_AUTO_DESCRIPTOR_HXX
