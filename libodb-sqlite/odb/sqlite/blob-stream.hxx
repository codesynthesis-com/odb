// file      : odb/sqlite/blob-stream.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_BLOB_STREAM_HXX
#define ODB_SQLITE_BLOB_STREAM_HXX

#include <odb/pre.hxx>

#include <odb/sqlite/blob.hxx>
#include <odb/sqlite/stream.hxx>

namespace odb
{
  namespace sqlite
  {
    class blob_stream: public stream
    {
    public:
      blob_stream (const blob& b, bool rw)
          : stream (b.db ().c_str (),
                    b.table ().c_str (),
                    b.column ().c_str (),
                    b.rowid (),
                    rw) {}
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_BLOB_STREAM_HXX
