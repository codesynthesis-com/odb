// file      : odb/qt/date-time/mysql/default-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MYSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_DATE_TIME_MYSQL_DEFAULT_MAPPING_HXX

#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>

// Map QDate to MySQL DATE by default. QDate provides a null
// representation so allow NULL values by default.
//
#pragma db value(QDate) type("DATE") null

// Map QTime to MySQL TIME by default. QTime provides a null
// representation so allow NULL values by default.
//
#pragma db value(QTime) type("TIME") null

// Map QDateTime to MySQL DATETIME by default. QDateTime provides a null
// representation so allow NULL values by default.
//
#pragma db value(QDateTime) type("DATETIME") null

#endif // ODB_QT_DATE_TIME_MYSQL_DEFAULT_MAPPING_HXX
