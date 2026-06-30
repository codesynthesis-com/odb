// file      : odb/details/lock.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_LOCK_HXX
#define ODB_DETAILS_LOCK_HXX

#include <odb/pre.hxx>

#include <odb/details/config.hxx>
#include <odb/details/mutex.hxx>

#ifdef ODB_THREADS_CXX11
#  include <mutex>
namespace odb
{
  namespace details
  {
    using lock = std::unique_lock<mutex>;
  }
}
#else
namespace odb
{
  namespace details
  {
    class unique_lock
    {
    public:
      explicit
      unique_lock (mutex& m)
          : mutex_ (m), locked_ (true)
      {
        mutex_.lock ();
      }

      ~unique_lock ()
      {
        if (locked_)
          mutex_.unlock ();
      }

      void
      unlock ()
      {
        mutex_.unlock ();
        locked_ = false;
      }

      void
      lock ()
      {
        mutex_.lock ();
        locked_ = true;
      }

    private:
      mutex& mutex_;
      bool locked_;
    };

    using lock = unique_lock;
  }
}
#endif

#include <odb/post.hxx>

#endif // ODB_DETAILS_LOCK_HXX
