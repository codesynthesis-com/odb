// file      : odb/pgsql/endian-traits.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/pgsql/endian-traits.hxx>

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
