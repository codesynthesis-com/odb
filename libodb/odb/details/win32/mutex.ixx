// file      : odb/details/win32/mutex.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace details
  {
    inline mutex::
    ~mutex ()
    {
      DeleteCriticalSection (&cs_);
    }

    inline mutex::
    mutex ()
    {
      InitializeCriticalSection (&cs_);
    }

    inline void mutex::
    lock ()
    {
      EnterCriticalSection (&cs_);
    }

    inline void mutex::
    unlock ()
    {
      LeaveCriticalSection (&cs_);
    }
  }
}
