// file      : odb/pgsql/auto-handle.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_AUTO_HANDLE_HXX
#define ODB_PGSQL_AUTO_HANDLE_HXX

#include <odb/pre.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/pgsql-fwd.hxx> // PGconn, PGresult

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename H>
    struct handle_traits;

    template <>
    struct LIBODB_PGSQL_EXPORT handle_traits<PGconn>
    {
      static void
      release (PGconn*);
    };

    template <>
    struct LIBODB_PGSQL_EXPORT handle_traits<PGresult>
    {
      static void
      release (PGresult*);
    };

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

      H*
      get () const
      {
        return h_;
      }

      void
      reset (H* h = 0)
      {
        if (h_ != 0)
          handle_traits<H>::release (h_);

        h_ = h;
      }

      H*
      release ()
      {
        H* h (h_);
        h_ = 0;
        return h;
      }

      operator H* () const
      {
        return h_;
      }

    private:
      auto_handle (const auto_handle&);
      auto_handle& operator= (const auto_handle&);

    private:
      H* h_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_AUTO_HANDLE_HXX
