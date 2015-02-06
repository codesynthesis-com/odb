// file      : odb/qt/basic/mysql/default-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QUuid>

// Map QString to MySQL TEXT for non-id and to VARCHAR(255) for id members.
// MySQL cannot have primary key of the TEXT type. Allow NULL values by
// default as QString provides a null representation.
//
#pragma db value(QString) type("TEXT") id_type("VARCHAR(255)") null

// Map QByteArray to MySQL BLOB by default. Allow NULL values by default as
// QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BLOB") null

// By default map QUuid to MySQL BINARY(16) and use NULL to represent null
// UUIDs. If NULL is disabled (e.g., at the member level), then we store
// the null UUID (i.e., all bytes are zero).
//
#pragma db value(QUuid) type("BINARY(16)") null

#endif // ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
