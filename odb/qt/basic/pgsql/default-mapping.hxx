// file      : odb/qt/basic/pgsql/default-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_PGSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_PGSQL_DEFAULT_MAPPING_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QUuid>

// Map QString to PostgreSQL TEXT by default. Allow NULL values by default as
// QString provides a null representation.
//
#pragma db value(QString) type("TEXT") null

// Map QByteArray to PostgreSQL BYTEA by default. Allow NULL values by default
// as QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BYTEA") null

// By default map QUuid to PostgreSQL UUID and use NULL to represent null
// UUIDs. If NULL is disabled (e.g., at the member level), then we store
// the null UUID (i.e., all bytes are zero).
//
#pragma db value(QUuid) type("UUID") null

#endif // ODB_QT_BASIC_PGSQL_DEFAULT_MAPPING_HXX
