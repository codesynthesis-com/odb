// file      : odb/qt/basic/oracle/default-mapping.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_ORACLE_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_ORACLE_DEFAULT_MAPPING_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>

// Map QString to Oracle VARCHAR2 by default. Allow NULL values by default as
// QString provides a null representation.
//
#pragma db value(QString) type("VARCHAR2(512)") null

// Map QByteArray to Oracle BLOB by default. Allow NULL values by default as
// QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BLOB") null

#endif // ODB_QT_BASIC_ORACLE_DEFAULT_MAPPING_HXX
