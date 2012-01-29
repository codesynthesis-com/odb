// file      : odb/qt/basic/mysql/default-mapping.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>

// Map QString to MySQL TEXT for non-id and to VARCHAR(256) for id members.
// MySQL cannot have primary key of the TEXT type. Allow NULL values by
// default as QString provides a null representation.
//
#pragma db value(QString) type("TEXT") id_type("VARCHAR(256)") null

// Map QByteArray to MySQL BLOB by default. Allow NULL values by default as
// QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BLOB") null

#endif // ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
