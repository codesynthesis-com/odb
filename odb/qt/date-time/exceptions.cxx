// file      : odb/qt/date-time/exceptions.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/qt/date-time/exceptions.hxx>

namespace odb
{
  namespace qt
  {
    namespace date_time
    {
      const char* value_out_of_range::
      what () const throw ()
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
