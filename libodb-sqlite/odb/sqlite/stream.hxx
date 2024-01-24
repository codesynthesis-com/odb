// file      : odb/sqlite/stream.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_STREAM_HXX
#define ODB_SQLITE_STREAM_HXX

#include <odb/pre.hxx>

#include <sqlite3.h>

#include <cstddef>   // std::size_t

#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    // SQLite incremental BLOB/TEXT I/O stream. Available since
    // SQLite 3.4.0.
    //
    class LIBODB_SQLITE_EXPORT stream: public active_object
    {
    public:
      // @@ TODO: db is actually what we now (and SQLite in other places)
      //    call schema (see database::schema(), ATTACH DATABASE). So we
      //    should probably rename this at some point for consistency.
      //
      stream (const char* db,
              const char* table,
              const char* column,
              long long rowid,
              bool rw);

      std::size_t
      size () const;

      // The following two functions throw std::invalid_argument if
      // offset + n is past size().
      //
      void
      read (void* buf, std::size_t n, std::size_t offset = 0);

      void
      write (const void* buf, std::size_t n, std::size_t offset = 0);

      sqlite3_blob*
      handle () const {return h_;}

      // Close without reporting errors, if any.
      //
      virtual
      ~stream () {close (false);}

      // Close, by default with reporting errors, if any.
      //
      void
      close (bool check = true);

      // Open the same BLOB but in a different row. Can be faster
      // than creating a new stream instance. Note that the stream
      // must be in the open state prior to calling this function.
      // Only available since SQLite 3.7.4.
      //
#if SQLITE_VERSION_NUMBER >= 3007004
      void
      reopen (long long rowid);
#endif

    protected:
      // The active_object interface.
      //
      virtual void
      clear ();

    private:
      sqlite3_blob* h_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_STREAM_HXX
