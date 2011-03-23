// file      : qt/mysql/default-mapping.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_MYSQL_DEFAULT_MAPPING_HXX

#include <QString>

// Map QString to MySQL TEXT by default.
//
#pragma db value(QString) type("TEXT")

#endif // ODB_QT_MYSQL_DEFAULT_MAPPING_HXX
