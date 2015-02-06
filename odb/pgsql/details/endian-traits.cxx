// file      : odb/pgsql/details/endian-traits.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/pgsql/details/endian-traits.hxx>

namespace odb
{
  namespace pgsql
  {
    namespace details
    {
      namespace
      {
        endian_traits::endian
        infer_host_endian ()
        {
          short s (1);
          char* c (reinterpret_cast<char*> (&s));

          return *c == 0 ?
            endian_traits::big :
            endian_traits::little;
        }
      }

      const endian_traits::endian endian_traits::host_endian (
        infer_host_endian ());
    }
  }
}
