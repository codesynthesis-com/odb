// file      : odb/qt/basic/sqlite/default-mapping.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_BASIC_SQLITE_DEFAULT_MAPPING_HXX
#define ODB_QT_BASIC_SQLITE_DEFAULT_MAPPING_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>

// Map QString to SQLite TEXT by default. Allow NULL values by default as
// QString provides a null representation.
//
#pragma db value(QString) type("TEXT") null

// Map QByteArray to SQLite BLOB by default. Allow NULL values by default as
// QByteArray provides a null representation.
//
#pragma db value(QByteArray) type("BLOB") null

#endif // ODB_QT_BASIC_SQLITE_DEFAULT_MAPPING_HXX
