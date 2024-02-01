// file      : odb/boost/date-time/exceptions.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/boost/date-time/exceptions.hxx>

namespace odb
{
  namespace boost
  {
    namespace date_time
    {
      const char* special_value::
      what () const ODB_NOTHROW_NOEXCEPT
      {
        return "unrepresentable date/time special value";
      }

      special_value* special_value::
      clone () const
      {
        return new special_value (*this);
      }

      const char* value_out_of_range::
      what () const ODB_NOTHROW_NOEXCEPT
      {
        return "date/time value out of range";
      }

      value_out_of_range* value_out_of_range::
      clone () const
      {
        return new value_out_of_range (*this);
      }
    }
  }
}
