// file      : odb/boost/date-time/mssql/posix-time-mapping.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_MSSQL_POSIX_TIME_MAPPING_HXX
#define ODB_BOOST_DATE_TIME_MSSQL_POSIX_TIME_MAPPING_HXX

#include <boost/date_time/posix_time/posix_time_types.hpp>

// By default map boost::posix_time::ptime to SQL Server DATETIME2
// (available only since SQL Server 2008). We use the NULL value to
// represent not_a_date_time.
//
#pragma db value(boost::posix_time::ptime) type("DATETIME2") null

// By default map boost::posix_time::time_duration to SQL Server TIME
// (available only since SQL Server 2008). We use the NULL value to
// represent not_a_date_time.
//
#pragma db value(boost::posix_time::time_duration) type("TIME") null

#endif // ODB_BOOST_DATE_TIME_MSSQL_POSIX_TIME_MAPPING_HXX
