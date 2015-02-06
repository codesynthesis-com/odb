// file      : odb/qt/date-time/mssql/default-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_MSSQL_DEFAULT_MAPPING_HXX
#define ODB_QT_DATE_TIME_MSSQL_DEFAULT_MAPPING_HXX

#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>

// By default map QDate to SQL Server DATE (available only since SQL
// Server 2008). QDate provides a null representation so allow NULL
// values by default.
//
#pragma db value(QDate) type("DATE") null

// By default map QTime to SQL Server TIME(3) (available only since SQL
// Server 2008). QTime can only represent clock times with a maximum
// precision of milliseconds. QTime provides a null representation so
// allow NULL values by default.
//
#pragma db value(QTime) type("TIME(3)") null

// By default map QDateTime to SQL Server DATETIME2(3) (available only
// since SQL Server 2008). QDateTime can only represent clock times with
// a maximum precision of milliseconds. QDateTime provides a null
// representation so allow NULL values by default.
//
#pragma db value(QDateTime) type("DATETIME2(3)") null

#endif // ODB_QT_DATE_TIME_MSSQL_DEFAULT_MAPPING_HXX
