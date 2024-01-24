// file      : odb/details/posix/thread.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/details/posix/exceptions.hxx>

namespace odb
{
  namespace details
  {
    inline thread::
    ~thread ()
    {
      if (!detached_)
        pthread_detach (id_);
    }

    inline void* thread::
    join ()
    {
      void* r;

      if (int e = pthread_join (id_, &r))
        throw posix_exception (e);

      detached_ = true;
      return r;
    }
  }
}
