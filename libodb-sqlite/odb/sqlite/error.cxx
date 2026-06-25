// file      : odb/sqlite/error.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/sqlite/error.hxx>

#include <sqlite3.h>

#include <new>    // std::bad_alloc
#include <string>

#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/exceptions.hxx>

#include <odb/sqlite/details/config.hxx> // LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY

using namespace std;

namespace odb
{
  namespace sqlite
  {
    void
    translate_error (int e, connection& c)
    {
      // Note: extended error codes are only available since SQLite 3.6.5.
      //
      translate_error (e, sqlite3_extended_errcode (c.handle ()), c);
    }

    void
    translate_error (int e, int ee, connection& c)
    {
      string m;

      switch (e)
      {
      case SQLITE_NOMEM:
        {
          throw bad_alloc ();
        }
      case SQLITE_MISUSE:
        {
          // In case of SQLITE_MISUSE, error code/message may or may not
          // be set.
          //
          ee = e;
          m = "SQLite API misuse";
          break;
        }
#ifdef SQLITE_ABORT_ROLLBACK
      case SQLITE_ABORT:
        {
          if (ee == SQLITE_ABORT_ROLLBACK)
            throw forced_rollback ();

          break;
        }
#endif
      case SQLITE_LOCKED:
        {
#ifdef LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY
          if (ee != SQLITE_LOCKED_SHAREDCACHE)
            throw deadlock (); // The DROP TABLE special case.
#endif
          // Getting SQLITE_LOCKED_SHAREDCACHE here means we don't have
          // the unlock notify support. Translate this to timeout.
          //
          throw timeout ();
        }
      case SQLITE_BUSY:
      case SQLITE_IOERR:
        {
          if (e != SQLITE_IOERR || ee == SQLITE_IOERR_BLOCKED)
            throw timeout ();

          break;
        }
      default:
        break;
      }

      if (m.empty ())
      {
        sqlite3* h (c.handle ());

        // Note that sqlite3_errmsg() returns more precise description
        // compared to sqlite3_errstr() but there is a chance it has been
        // overwritten. So do the best we can.
        //
        const char* s;
        if (sqlite3_errcode (h) == e)
          s = sqlite3_errmsg (h);
        else
          s = sqlite3_errstr (e);

        m = s != nullptr ? s : "unknown error";
      }

      // Get rid of a trailing newline if there is one.
      //
      string::size_type n (m.size ());
      if (n != 0 && m[n - 1] == '\n')
        m.resize (n - 1);

      throw database_exception (e, ee, m);
    }
  }
}
