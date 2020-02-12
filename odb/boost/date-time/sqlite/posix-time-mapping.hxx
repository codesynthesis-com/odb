// file      : odb/boost/date-time/sqlite/posix-time-mapping.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DATE_TIME_SQLITE_POSIX_TIME_MAPPING_HXX
#define ODB_BOOST_DATE_TIME_SQLITE_POSIX_TIME_MAPPING_HXX

#include <boost/date_time/posix_time/posix_time_types.hpp>

// By default map boost::posix_time::ptime to TEXT. We use
// the NULL value to represent not_a_date_time.
//
#pragma db value(boost::posix_time::ptime) type("TEXT") null

// By default map boost::posix_time::time_duration to TEXT. We
// use the NULL value to represent not_a_date_time.
//
#pragma db value(boost::posix_time::time_duration) type("TEXT") null

#endif // ODB_BOOST_DATE_TIME_SQLITE_POSIX_TIME_MAPPING_HXX
