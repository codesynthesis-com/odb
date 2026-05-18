// file      : odb/details/transfer-ptr.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_TRANSFER_PTR_HXX
#define ODB_DETAILS_TRANSFER_PTR_HXX

#include <odb/pre.hxx>

#include <memory> // std::unique_ptr

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
      transfer_ptr (std::nullptr_t): p_ (0) {}

      template <typename T1>
      transfer_ptr (std::unique_ptr<T1>&& p): p_ (p.release ()) {}

    private:
      transfer_ptr (const transfer_ptr&);
      transfer_ptr& operator= (const transfer_ptr&);

    public:
      transfer_ptr (transfer_ptr&& p) noexcept: p_ (p.transfer ()) {}

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
