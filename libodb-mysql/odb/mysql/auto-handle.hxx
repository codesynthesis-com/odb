// file      : odb/mysql/auto-handle.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_AUTO_HANDLE_HXX
#define ODB_MYSQL_AUTO_HANDLE_HXX

#include <odb/pre.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/mysql.hxx>

namespace odb
{
  namespace mysql
  {
    template <typename H>
    struct handle_traits;

    template <>
    struct handle_traits<MYSQL>
    {
      static void
      release (MYSQL* h)
      {
        mysql_close (h);
      }
    };

    template <>
    struct handle_traits<MYSQL_STMT>
    {
      static void
      release (MYSQL_STMT* h)
      {
        mysql_stmt_close (h);
      }
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

#endif // ODB_MYSQL_AUTO_HANDLE_HXX
