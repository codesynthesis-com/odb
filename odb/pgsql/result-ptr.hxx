// file      : odb/pgsql/result-ptr.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_RESULT_PTR_HXX
#define ODB_PGSQL_RESULT_PTR_HXX

#include <odb/pre.hxx>

#include <libpq-fe.h>

#include <odb/pgsql/version.hxx>

#include <odb/pgsql/details/export.hxx>

//
// Note: do not include this header into public headers (i.e., those
// that may be included, directly or indirectly, by user code) because
// it includes libpq-fe.h
//

namespace odb
{
  namespace pgsql
  {
    class LIBODB_PGSQL_EXPORT result_ptr
    {
    public:
      result_ptr (PGresult* r = 0)
          : r_ (r)
      {
      }

      ~result_ptr ()
      {
        if (r_ != 0)
          PQclear (r_);
      }

      PGresult*
      get () const
      {
        return r_;
      }

      void
      reset (PGresult* r = 0)
      {
        if (r_ != 0)
          PQclear (r_);

        r_ = r;
      }

      PGresult*
      release ()
      {
        PGresult* r (r_);
        r_ = 0;
        return r;
      }

    private:
      result_ptr (const result_ptr&);
      result_ptr& operator= (const result_ptr&);

    private:
      PGresult* r_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_RESULT_PTR_HXX
