// file      : odb/mssql/database.ixx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace mssql
  {
    inline connection_ptr database::
    connection ()
    {
      // Go through the virtual connection_() function instead of
      // directly to allow overriding.
      //
      return connection_ptr (
        static_cast<mssql::connection*> (connection_ ()));
    }
  }
}
