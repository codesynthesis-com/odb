// file      : libcommon/concrete.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_CONCRETE_HXX
#define LIBCOMMON_CONCRETE_HXX

#include <libcommon/config.hxx>

// Namespace alias for the concrete database namespace.
//
#if defined(MULTI_DATABASE)

// Fallback to common interface.
//
#include <odb/database.hxx>
#include <odb/transaction.hxx>

namespace odb_db = odb;

#elif defined(DATABASE_MYSQL)

#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>

namespace odb_db = odb::mysql;

#elif defined(DATABASE_SQLITE)

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

namespace odb_db = odb::sqlite;

#elif defined(DATABASE_PGSQL)

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

namespace odb_db = odb::pgsql;

#elif defined(DATABASE_ORACLE)

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

namespace odb_db = odb::oracle;

#elif defined(DATABASE_MSSQL)

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

namespace odb_db = odb::mssql;

#endif

#endif // LIBCOMMON_CONCRETE_HXX
