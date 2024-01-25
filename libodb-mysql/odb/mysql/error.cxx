// file      : odb/mysql/error.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <new>    // std::bad_alloc
#include <string>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
    static void
    translate_error (connection& c,
                     unsigned int e,
                     const string& sqlstate,
                     string msg)
    {
      switch (e)
      {
      case CR_OUT_OF_MEMORY:
        {
          throw bad_alloc ();
        }
      case ER_LOCK_DEADLOCK:
        {
          throw deadlock ();
        }
      case CR_SERVER_LOST:
      case CR_SERVER_GONE_ERROR:
        {
          c.mark_failed ();
          throw connection_lost ();
        }
      case CR_UNKNOWN_ERROR:
        {
          c.mark_failed ();
        }
        // Fall through.
      default:
        {
          // Get rid of a trailing newline if there is one.
          //
          string::size_type n (msg.size ());
          if (n != 0 && msg[n - 1] == '\n')
            msg.resize (n - 1);

          throw database_exception (e, sqlstate, msg);
        }
      }
    }

    void
    translate_error (connection& c)
    {
      MYSQL* h (c.handle ());
      translate_error (c,
                       mysql_errno (h),
                       mysql_sqlstate (h),
                       mysql_error (h));
    }

    void
    translate_error (connection& c, MYSQL_STMT* h)
    {
      translate_error (c,
                       mysql_stmt_errno (h),
                       mysql_stmt_sqlstate (h),
                       mysql_stmt_error (h));
    }
  }
}
