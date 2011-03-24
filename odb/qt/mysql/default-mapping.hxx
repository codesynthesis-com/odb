// file      : qt/mysql/default-mapping.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_MYSQL_DEFAULT_MAPPING_HXX

#include <QString>
#include <QDate>
#include <QTime>
#include <QDateTime>

// Map QString to MySQL TEXT by default.
//
#pragma db value(QString) type("VARCHAR(56) NOT NULL")

// Map QDate to MySQL DATE by default.
//
#pragma db value(QDate) type("DATE")

// Map QTime to MySQL TIME by default.
//
#pragma db value(QTime) type("TIME")

// Map QTime to MySQL DATETIME by default.
//
#pragma db value(QDateTime) type("DATETIME")

#endif // ODB_QT_MYSQL_DEFAULT_MAPPING_HXX
