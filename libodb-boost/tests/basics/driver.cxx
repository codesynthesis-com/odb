// file      : tests/basics/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Basic test to make sure the library is usable. Functionality testing
// is done in the odb-tests package.

#include <odb/boost/exception.hxx>
#include <odb/boost/date-time/exceptions.hxx>

using namespace odb;

int
main ()
{
  try
  {
    throw boost::date_time::value_out_of_range ();
  }
  catch (const boost::exception&)
  {
  }
}
