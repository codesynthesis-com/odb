// file      : odb/mysql/enums.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstring> // std::memmove
#include <cassert>

#include <odb/mysql/enum.hxx>

namespace odb
{
  namespace mysql
  {
    void enum_traits::
    strip_value (const details::buffer& i, unsigned long& size)
    {
      char* d (const_cast<char*> (i.data ()));

      unsigned long p (0);
      for (; p < size && d[p] != ' '; ++p) ;
      assert (p != size);

      p++; // Skip space;
      size -= p;

      std::memmove (d, d + p, size);
    }
  }
}
