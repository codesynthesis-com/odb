// file      : odb/qt/basic/mssql/default-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MSSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_MSSQL_DEFAULT_MAPPING_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QUuid>

// By default map QString to SQL Server VARCHAR(512) for non-id members
// and to VARCHAR(256) for id members (the same as the default mapping
// for std::string). Allow NULL values by default as QString provides
// a null representation.
//
#pragma db value(QString) type("VARCHAR(512)") id_type("VARCHAR(256)") null

// By default map QByteArray to SQL Server VARBINARY(max). Allow NULL
// values by default as QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("VARBINARY(max)") null

// By default map QUuid to SQL Server UNIQUEIDENTIFIER and use NULL to
// represent null UUIDs. If NULL is disabled (e.g., at the member level),
// then we store the null UUID (i.e., all bytes are zero).
//
#pragma db value(QUuid) type("UNIQUEIDENTIFIER") null

#endif // ODB_QT_BASIC_MSSQL_DEFAULT_MAPPING_HXX
