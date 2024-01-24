// file      : odb/sqlite/stream.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sqlite3.h>

#if SQLITE_VERSION_NUMBER >= 3004000

#include <odb/sqlite/stream.hxx>

#include <stdexcept> // invalid_argument

#include <odb/sqlite/error.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

using namespace std;

namespace odb
{
  namespace sqlite
  {
    stream::
    stream (const char* db,
            const char* table,
            const char* column,
            long long rowid,
            bool rw)
        : active_object (transaction::current ().connection ())
    {
      int e (sqlite3_blob_open (conn_.handle (),
                                db,
                                table,
                                column,
                                static_cast<sqlite_int64> (rowid),
                                rw,
                                &h_));

      if (e != SQLITE_OK)
        translate_error (e, conn_);

      list_add (); // Add ourselves to the active objects list.
    }

    size_t stream::
    size () const
    {
      return static_cast<size_t> (sqlite3_blob_bytes (h_));
    }

    void stream::
    read (void* buf, size_t n, size_t o)
    {
      int e (sqlite3_blob_read (
               h_, buf, static_cast<int> (n), static_cast<int> (o)));

      if (e != SQLITE_OK)
      {
        if (e == SQLITE_ERROR)
          throw invalid_argument ("read past end");
        else
          translate_error (e, conn_);
      }
    }

    void stream::
    write (const void* buf, size_t n, size_t o)
    {
      int e (sqlite3_blob_write (
               h_, buf, static_cast<int> (n), static_cast<int> (o)));

      if (e != SQLITE_OK)
      {
        if (e == SQLITE_ERROR)
          throw invalid_argument ("write past end");
        else
          translate_error (e, conn_);
      }
    }

    void stream::
    close (bool check)
    {
      if (h_ != 0)
      {
        list_remove ();

        int e (sqlite3_blob_close (h_));
        h_ = 0; // No use trying again.

        if (check && e != SQLITE_OK)
          translate_error (e, conn_);
      }
    }

#if SQLITE_VERSION_NUMBER >= 3007004
    void stream::
    reopen (long long rowid)
    {
      int e (sqlite3_blob_reopen (h_, rowid));

      if (e != SQLITE_OK)
        // According to the SQLite documentation, the handle is now
        // considered aborted, which means we still need to close it.
        //
        translate_error (e, conn_);
    }
#endif

    void stream::
    clear ()
    {
      // This call can be part of the stack unwinding, so don't check
      // for the errors.
      //
      close (false);
    }
  }
}

#endif // SQLITE_VERSION_NUMBER >= 3004000
