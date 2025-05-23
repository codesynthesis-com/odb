// file      : odb/mssql/auto-handle.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_AUTO_HANDLE_HXX
#define ODB_MSSQL_AUTO_HANDLE_HXX

#include <odb/pre.hxx>

#include <odb/details/config.hxx> // ODB_CXX11

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    LIBODB_MSSQL_EXPORT void
    free_handle (SQLHANDLE, SQLSMALLINT htype);

    template <SQLSMALLINT htype>
    class auto_handle
    {
    public:
      auto_handle (SQLHANDLE h = 0)
          : h_ (h)
      {
      }

      ~auto_handle ()
      {
        if (h_ != 0)
          free_handle (h_, htype);
      }

      operator SQLHANDLE () const
      {
        return h_;
      }

      SQLHANDLE
      get () const
      {
        return h_;
      }

      SQLHANDLE
      release ()
      {
        SQLHANDLE h (h_);
        h_ = 0;
        return h;
      }

      void
      reset (SQLHANDLE h = 0)
      {
        if (h_ != 0)
          free_handle (h_, htype);

        h_ = h;
      }

#ifdef ODB_CXX11
      auto_handle (auto_handle&& ah) noexcept: h_ (ah.release ()) {}
      auto_handle& operator= (auto_handle&& ah) noexcept
      {
        if (this != &ah)
          reset (ah.release ());
        return *this;
      }
#endif

    private:
      auto_handle (const auto_handle&);
      auto_handle& operator= (const auto_handle&);

    private:
      SQLHANDLE h_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_AUTO_HANDLE_HXX
