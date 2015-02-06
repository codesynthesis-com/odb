// file      : odb/oracle/auto-handle.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_AUTO_HANDLE_HXX
#define ODB_ORACLE_AUTO_HANDLE_HXX

#include <odb/pre.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-fwd.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    //
    // handle_type_traits
    //

    template <typename H>
    struct handle_type_traits;

    template <>
    struct LIBODB_ORACLE_EXPORT handle_type_traits<OCIEnv>
    { static const ub4 htype; };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_type_traits<OCIError>
    { static const ub4 htype; };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_type_traits<OCISvcCtx>
    { static const ub4 htype; };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_type_traits<OCIStmt>
    { static const ub4 htype; };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_type_traits<OCIAuthInfo>
    { static const ub4 htype; };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_type_traits<OCITrans>
    { static const ub4 htype; };

    //
    // handle_traits
    //

    LIBODB_ORACLE_EXPORT void
    oci_handle_free (void* handle, ub4 type);

    template <typename H>
    struct handle_traits
    {
      static void
      release (H* h)
      {
        oci_handle_free (h, handle_type_traits<H>::htype);
      }
    };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_traits<OCISvcCtx>
    {
      static void
      release (OCISvcCtx*, OCIError*);
    };

    template <>
    struct LIBODB_ORACLE_EXPORT handle_traits<OCIStmt>
    {
      static void
      release (OCIStmt*, ub4 release_mode, OCIError*);
    };

    //
    // auto_handle
    //

    template <typename H>
    class auto_handle
    {
    public:
      auto_handle (H* h = 0)
          : h_ (h)
      {
      }

      ~auto_handle ()
      {
        if (h_ != 0)
          handle_traits<H>::release (h_);
      }

      operator H* () const
      {
        return h_;
      }

      H*
      get () const
      {
        return h_;
      }

      H*
      release ()
      {
        H* h (h_);
        h_ = 0;
        return h;
      }

      void
      reset (H* h = 0)
      {
        if (h_ != 0)
          handle_traits<H>::release (h_);

        h_ = h;
      }

    private:
      auto_handle (const auto_handle&);
      auto_handle& operator= (const auto_handle&);

    private:
      H* h_;
    };

    //
    // auto_handle<OCISvcCtx>
    //

    template <>
    class LIBODB_ORACLE_EXPORT auto_handle<OCISvcCtx>
    {
    public:
      auto_handle ()
        : h_ (0)
      {
      }

      auto_handle (OCISvcCtx* h, OCIError* e)
        : h_ (h), e_ (e)
      {
      }

      ~auto_handle ()
      {
        if (h_ != 0)
          handle_traits<OCISvcCtx>::release (h_, e_);
      }

      operator OCISvcCtx* () const
      {
        return h_;
      }

      OCISvcCtx*
      get () const
      {
        return h_;
      }

      OCISvcCtx*
      release ()
      {
        OCISvcCtx* h (h_);
        h_ = 0;

        return h;
      }

      void
      reset ()
      {
        if (h_ != 0)
        {
          handle_traits<OCISvcCtx>::release (h_, e_);
          h_ = 0;
        }
      }

      void
      reset (OCISvcCtx* h, OCIError* e)
      {
        if (h_ != 0)
          handle_traits<OCISvcCtx>::release (h_, e_);

        h_ = h;
        e_ = e;
      }

    private:
      OCISvcCtx* h_;
      OCIError* e_;
    };

    //
    // auto_handle<OCIStmt>
    //

    template <>
    class LIBODB_ORACLE_EXPORT auto_handle<OCIStmt>
    {
    public:
      auto_handle ()
        : h_ (0)
      {
      }

      auto_handle (OCIStmt* h, ub4 release_mode, OCIError* e)
        : h_ (h), release_mode_ (release_mode), e_ (e)
      {
      }

      ~auto_handle ()
      {
        if (h_ != 0)
          handle_traits<OCIStmt>::release (h_, release_mode_, e_);
      }

      operator OCIStmt* () const
      {
        return h_;
      }

      OCIStmt*
      get () const
      {
        return h_;
      }

      void
      reset ()
      {
        if (h_ != 0)
        {
          handle_traits<OCIStmt>::release (h_, release_mode_, e_);
          h_ = 0;
        }
      }

      void
      reset (OCIStmt* h, ub4 release_mode, OCIError* e)
      {
        if (h_ != 0)
          handle_traits<OCIStmt>::release (h_, release_mode_, e_);

        h_ = h;
        release_mode_ = release_mode;
        e_ = e;
      }

    private:
      OCIStmt* h_;
      ub4 release_mode_;
      OCIError* e_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_AUTO_HANDLE_HXX
