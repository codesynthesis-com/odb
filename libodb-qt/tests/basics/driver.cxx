// file      : tests/basics/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Basic test to make sure the library is usable. Functionality testing
// is done in the odb-tests package.

#include <odb/qt/exception.hxx>
#include <odb/qt/date-time/exceptions.hxx>

using namespace odb;

int
main ()
{
  try
  {
    throw qt::date_time::value_out_of_range ();
  }
  catch (const qt::exception&)
  {
  }
}
