// file      : odb/oracle/statements-base.hxx
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_STATEMENTS_BASE_HXX
#define ODB_ORACLE_STATEMENTS_BASE_HXX

#include <odb/pre.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx> // connection

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class LIBODB_ORACLE_EXPORT statements_base: public details::shared_base
    {
    public:
      typedef oracle::connection connection_type;

      connection_type&
      connection ()
      {
        return conn_;
      }

    public:
      virtual
      ~statements_base ();

    protected:
      statements_base (connection_type& conn)
        : conn_ (conn)
      {
      }

    protected:
      connection_type& conn_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_STATEMENTS_BASE_HXX
