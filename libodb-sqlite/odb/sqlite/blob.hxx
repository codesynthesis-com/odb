// file      : odb/sqlite/blob.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_BLOB_HXX
#define ODB_SQLITE_BLOB_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef> // std::size_t

// Carefully allow this header to be included into the ODB compilation.
//
#ifndef ODB_COMPILER
#  include <odb/sqlite/forward.hxx>
#endif

namespace odb
{
  namespace sqlite
  {
#ifdef ODB_COMPILER
    #pragma db sqlite:type("BLOB STREAM")
    class blob
#else
    class blob
#endif
    {
    public:
      // BLOB size to provision for. Set before calling persist() or update().
      //
      explicit
      blob (std::size_t size = 0): size_ (size), rowid_ (0) {}

      std::size_t size () const {return size_;}
      void size (std::size_t s) {size_ = s;}

      const std::string& db () const {return db_;}
      const std::string& table () const {return table_;}
      const std::string& column () const {return column_;}
      long long rowid () const {return rowid_;}

      void
      clear ()
      {
        size_ = 0;
        db_.clear ();
        table_.clear ();
        column_.clear ();
        rowid_ = 0;
      }

    private:
#ifndef ODB_COMPILER
      friend struct default_value_traits<blob, id_blob_stream>;
#endif
      std::size_t size_;
      mutable std::string db_;
      mutable std::string table_;
      mutable std::string column_;
      mutable long long rowid_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_BLOB_HXX
