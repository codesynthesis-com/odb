// file      : odb/qt/basic/mysql/default-mapping.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX

#include <QString>
#include <QByteArray>

// By default, map QString to MySQL TEXT for non-id and to VARCHAR(256) for
// id members. MySQL cannot have primary keys of the TEXT type.
//
#pragma db value(QString) type("TEXT NOT NULL") \
  id_type("VARCHAR(256) NOT NULL")

// Map QByteArray to MySQL BLOB by default. Allow NULL values by default as
// QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BLOB")

#endif // ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
