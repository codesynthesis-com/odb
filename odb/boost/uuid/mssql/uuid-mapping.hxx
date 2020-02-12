// file      : odb/boost/uuid/mssql/uuid-mapping.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_UUID_MSSQL_UUID_MAPPING_HXX
#define ODB_BOOST_UUID_MSSQL_UUID_MAPPING_HXX

#include <boost/version.hpp>

// UUID library is available since 1.42.0.
//
#if BOOST_VERSION >= 104200

#include <boost/uuid/uuid.hpp>

// By default map boost::uuids::uuid to SQL Server UNIQUEIDENTIFIER and
// use NULL to represent nil UUIDs. If NULL is disabled (e.g., at the
// member level), then we store the nil UUID (i.e., all bytes are zero).
//
#pragma db value(boost::uuids::uuid) type("UNIQUEIDENTIFIER") null

#endif // BOOST_VERSION
#endif // ODB_BOOST_UUID_MSSQL_UUID_MAPPING_HXX
