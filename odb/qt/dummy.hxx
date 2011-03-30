// Force compilation and creation of libodb-qt library for now.
//
#include <QString>
#include <odb/qt/details/export.hxx>

namespace odb
{
  namespace qt
  {
    LIBODB_QT_EXPORT QString dummy ();
  }
}
