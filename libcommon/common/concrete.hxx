// file      : libcommon/common/concrete.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_CONCRETE_HXX
#define LIBCOMMON_COMMON_CONCRETE_HXX

#include <common/config.hxx>

// Namespace alias for the concrete database namespace.
//
#if defined(DATABASE_MYSQL)

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

#endif // LIBCOMMON_COMMON_CONCRETE_HXX
