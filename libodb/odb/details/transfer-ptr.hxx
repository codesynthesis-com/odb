// file      : odb/details/transfer-ptr.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_TRANSFER_PTR_HXX
#define ODB_DETAILS_TRANSFER_PTR_HXX

#include <odb/pre.hxx>

#include <memory> // std::auto_ptr, std::unique_ptr

#include <odb/details/config.hxx> // ODB_CXX11

namespace odb
{
  namespace details
  {
    template <typename T>
    class transfer_ptr
    {
    public:
      typedef T element_type;

      transfer_ptr (): p_ (0) {}

#ifndef ODB_CXX11
      template <typename T1>
      transfer_ptr (std::auto_ptr<T1> p): p_ (p.release ()) {}

    private:
      transfer_ptr& operator= (const transfer_ptr&);

    public:
      // In our usage transfer_ptr is always created implicitly and
      // never const. So while this is not very clean, it is legal.
      // Plus it will all go away once we drop C++98 (I can hardly
      // wait).
      //
      transfer_ptr (const transfer_ptr& p)
          : p_ (const_cast<transfer_ptr&> (p).transfer ()) {}
#else
#ifdef ODB_CXX11_NULLPTR
      transfer_ptr (std::nullptr_t): p_ (0) {}
#endif
      template <typename T1>
      transfer_ptr (std::unique_ptr<T1>&& p): p_ (p.release ()) {}

    private:
      transfer_ptr (const transfer_ptr&);
      transfer_ptr& operator= (const transfer_ptr&);

    public:
      transfer_ptr (transfer_ptr&& p) noexcept: p_ (p.transfer ()) {}
#endif

      ~transfer_ptr () {delete p_;}

      T*
      transfer ()
      {
        T* r (p_);
        p_ = 0;
        return r;
      }

    private:
      T* p_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_DETAILS_TRANSFER_PTR_HXX
