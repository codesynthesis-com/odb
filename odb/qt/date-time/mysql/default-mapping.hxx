// file      : odb/qt/date-time/mysql/default-mapping.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_DATE_TIME_MYSQL_DEFAULT_MAPPING_HXX

#include <QDate>
#include <QTime>
#include <QDateTime>

// Map QDate to MySQL DATE by default. QDate provides a null
// representation so allow NULL values by default.
//
#pragma db value(QDate) type("DATE")

// Map QTime to MySQL TIME by default. QTime provides a null
// representation so allow NULL values by default.
//
#pragma db value(QTime) type("TIME")

// Map QDateTime to MySQL DATETIME by default. QDateTime provides a null
// representation so allow NULL values by default.
//
#pragma db value(QDateTime) type("DATETIME")

#endif // ODB_QT_DATE_TIME_MYSQL_DEFAULT_MAPPING_HXX
