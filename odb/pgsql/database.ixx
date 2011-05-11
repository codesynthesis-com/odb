// file      : odb/pgsql/database.ixx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace pgsql
  {
    inline details::shared_ptr<database::connection_type> database::
    connection ()
    {
      return factory_->connect ();
    }
  }
}
