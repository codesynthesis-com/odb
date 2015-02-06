// file      : odb/qt/date-time/oracle/default-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_ORACLE_DEFAULT_MAPPING_HXX
#define ODB_QT_DATE_TIME_ORACLE_DEFAULT_MAPPING_HXX

#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>

// Map QDate to Oracle DATE by default. QDate provides a null representation
// so allow NULL values by default.
//
#pragma db value(QDate) type("DATE") null

// Map QTime to Oracle INTERVAL DAY(0) TO SECOND(3) by default. QTime can
// only represent clock times with a maximum precision of milliseconds.
// QTime provides a null representation so allow NULL values by default.
//
#pragma db value(QTime) type("INTERVAL DAY(0) TO SECOND(3)") null

// Map QDateTime to Oracle TIMESTAMP(3) by default. QDateTime can only
// represent clock times with a maximum precision of milliseconds.
// QDateTime provides a null representation so allow NULL values by
// default.
//
#pragma db value(QDateTime) type("TIMESTAMP(3)") null

#endif // ODB_QT_DATE_TIME_ORACLE_DEFAULT_MAPPING_HXX
