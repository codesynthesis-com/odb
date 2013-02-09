// file      : odb/mssql/statement.ixx
// copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace mssql
  {
    inline unsigned long long insert_statement::
    version ()
    {
      unsigned long long r;

      // The value is in the big-endian format.
      //
      unsigned char* p (reinterpret_cast<unsigned char*> (&r));
      p[0] = version_[7];
      p[1] = version_[6];
      p[2] = version_[5];
      p[3] = version_[4];
      p[4] = version_[3];
      p[5] = version_[2];
      p[6] = version_[1];
      p[7] = version_[0];

      return r;
    }

    inline unsigned long long update_statement::
    version ()
    {
      unsigned long long r;

      // The value is in the big-endian format.
      //
      unsigned char* p (reinterpret_cast<unsigned char*> (&r));
      p[0] = version_[7];
      p[1] = version_[6];
      p[2] = version_[5];
      p[3] = version_[4];
      p[4] = version_[3];
      p[5] = version_[2];
      p[6] = version_[1];
      p[7] = version_[0];

      return r;
    }
  }
}
