// file      : odb/qt/basic/mysql/default-mapping.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX

#include <QString>
#include <QByteArray>

// Map QString to MySQL VARCHAR (56) by default.
//
// @@ Temporary mapping until a solution for specifying primary key type
//    has been implemented.
//
#pragma db value(QString) type("VARCHAR(56) NOT NULL")

// Map QByteArray to MySQL BLOB by default. Allow NULL values by default as
// QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BLOB")

#endif // ODB_QT_BASIC_MYSQL_DEFAULT_MAPPING_HXX
