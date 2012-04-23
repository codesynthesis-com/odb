// file      : odb/oracle/binding.hxx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_BINDING_HXX
#define ODB_ORACLE_BINDING_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-types.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class LIBODB_ORACLE_EXPORT binding
    {
    public:
      typedef oracle::bind bind_type;
      typedef oracle::change_callback change_callback_type;

      binding (): bind (0), count (0), version (0), change_callback (0) {}

      binding (bind_type* b, std::size_t n)
        : bind (b), count (n), version (0), change_callback (0)
      {
      }

      bind_type* bind;
      std::size_t count;
      std::size_t version;

      change_callback_type* change_callback;

    private:
      binding (const binding&);
      binding& operator= (const binding&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_BINDING_HXX
