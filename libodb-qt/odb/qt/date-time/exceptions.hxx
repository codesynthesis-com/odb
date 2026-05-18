// file      : odb/qt/date-time/exceptions.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DATE_TIME_EXCEPTIONS_HXX
#define ODB_QT_DATE_TIME_EXCEPTIONS_HXX

#include <odb/pre.hxx>

#include <odb/qt/exception.hxx>
#include <odb/qt/details/export.hxx>

namespace odb
{
  namespace qt
  {
    namespace date_time
    {
      struct LIBODB_QT_EXPORT value_out_of_range: exception
      {
        virtual const char*
        what () const noexcept;

        virtual value_out_of_range*
        clone () const;
      };
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_QT_DATE_TIME_EXCEPTIONS_HXX
