// file      : odb/mysql/binding.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_BINDING_HXX
#define ODB_MYSQL_BINDING_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/forward.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/mysql-types.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class LIBODB_MYSQL_EXPORT binding
    {
    public:
      binding (): bind (0), count (0), version (0) {}

      binding (MYSQL_BIND* b, std::size_t n)
          : bind (b), count (n), version (0)
      {
      }

      MYSQL_BIND* bind;
      std::size_t count;
      std::size_t version;

    private:
      binding (const binding&);
      binding& operator= (const binding&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_BINDING_HXX
