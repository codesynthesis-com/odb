// file      : odb/pgsql/database.ixx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace pgsql
  {
    inline connection_ptr database::
    connection ()
    {
      // Go through the virtual connection_() function instead of
      // directly to allow overriding.
      //
      return connection_ptr (
        static_cast<pgsql::connection*> (connection_ ()));
    }
  }
}
