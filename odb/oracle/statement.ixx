// file      : odb/oracle/statement.ixx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

using namespace std;

namespace odb
{
  namespace oracle
  {
    // bulk_statement
    //
    inline bulk_statement::
    bulk_statement (connection_type& c,
                    const std::string& text,
                    statement_kind k,
                    const binding* process,
                    bool optimize,
                    std::size_t batch,
                    sb4* status)
        : statement (c, text, k, process, optimize),
          status_ (batch == 1 ? 0 : status)
    {
    }

    inline bulk_statement::
    bulk_statement (connection_type& c,
                    const char* text,
                    statement_kind k,
                    const binding* process,
                    bool optimize,
                    std::size_t batch,
                    sb4* status)
        : statement (c, text, k, process, optimize),
          status_ (batch == 1 ? 0 : status)
    {
    }

    // insert_statement
    //
    inline void insert_statement::
    fetch (sword r, sb4 code)
    {
      result_ = true;

      if (r != 0 /*OCI_SUCCESS*/)
      {
        // An auto-assigned object id should never cause a duplicate primary
        // key.
        //
        if (ret_ == 0)
        {
          // The Oracle error code ORA-00001 indicates unique constraint
          // violation, which covers more than just a duplicate primary key.
          // Unfortunately, there is nothing more precise that we can use.
          //
          if (code == 1)
            result_ = false;
        }
      }
    }
  }
}
