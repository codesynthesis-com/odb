// file      : libcommon/common.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_HXX
#define LIBCOMMON_COMMON_HXX

#include <memory>  // std::unique_ptr
#include <cstddef> // std::size_t

#include <odb/result.hxx>
#include <odb/database.hxx>

#include <libcommon/export.hxx>

LIBCOMMON_SYMEXPORT std::unique_ptr<odb::database>
create_database (int argc,
                 char* argv[],
                 bool create_schema = true,
                 std::size_t max_connections = 0,
                 odb::database_id db = odb::id_common);

template <typename T>
std::unique_ptr<T>
create_specific_database (int argc,
                          char* argv[],
                          bool create_schema = true,
                          std::size_t max_connections = 0)
{
  std::unique_ptr<odb::database> r (
    create_database (argc, argv,
                     create_schema,
                     max_connections,
                     T::database_id));

  return std::unique_ptr<T> (&dynamic_cast<T&> (*r.release ()));
}

LIBCOMMON_SYMEXPORT std::string
quote_name (const std::string&);

// This function returns an accurate result only if the result iterator
// hasn't been advanced and after the call the result is no longer valid.
//
template <typename T>
std::size_t
size (odb::result<T>);

#include <libcommon/common.txx>

#endif // LIBCOMMON_COMMON_HXX
