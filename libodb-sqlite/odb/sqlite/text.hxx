// file      : odb/sqlite/text.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_TEXT_HXX
#define ODB_SQLITE_TEXT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef> // std::size_t

// Carefully allow this header to be included into the ODB compilation.
//
#ifndef ODB_COMPILER
#  include <odb/sqlite/forward.hxx>
#  include <odb/sqlite/details/export.hxx>
#endif

namespace odb
{
  namespace sqlite
  {
#ifdef ODB_COMPILER
    #pragma db sqlite:type("TEXT STREAM")
    class text
#else
    class text
#endif
    {
    public:
      // TEXT size to provision for. Set before calling persist() or update().
      //
      explicit
      text (std::size_t size = 0): size_ (size) {}

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
      friend struct default_value_traits<text, id_text_stream>;
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

#endif // ODB_SQLITE_TEXT_HXX
