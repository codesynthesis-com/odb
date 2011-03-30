#include <odb/qt/dummy.hxx>
#include <odb/qt/exception.hxx>

namespace odb
{
  namespace qt
  {
    QString dummy ()
    {
      QString s ("Hello, Qt!");
      return s;
    }
  }
}
