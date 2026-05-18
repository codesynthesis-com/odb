// file      : odb/details/shared-ptr/base.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace details
  {
    // share
    //

    inline share::
    share (char id)
        : id_ (id)
    {
    }

    inline bool share::
    operator== (share x) const
    {
      return id_ == x.id_;
    }

    // shared_base
    //

    inline shared_base::
    shared_base ()
        : counter_ (1), callback_ (0)
    {
    }

    inline shared_base::
    shared_base (const shared_base&)
        : counter_ (1), callback_ (0)
    {
    }

    inline shared_base& shared_base::
    operator= (const shared_base&)
    {
      return *this;
    }

    inline void shared_base::
    _inc_ref ()
    {
      counter_.fetch_add (1, std::memory_order_relaxed);
    }

    inline bool shared_base::
    _dec_ref ()
    {
      // While there are ways to avoid acquire (which is unnecessary except
      // when the counter drops to zero), for our use-cases we'd rather keep
      // it simple.
      //
      return
        counter_.fetch_sub (1, std::memory_order_acq_rel) == 1
        ? callback_ == 0 || callback_->zero_counter (callback_->arg)
        : false;
    }

    inline std::size_t shared_base::
    _ref_count () const
    {
      return counter_.load (std::memory_order_relaxed);
    }

    inline void* shared_base::
    operator new (std::size_t n)
    {
      return ::operator new (n);
    }

    inline void* shared_base::
    operator new (std::size_t n, share)
    {
      return ::operator new (n);
    }

    inline void shared_base::
    operator delete (void* p, share) noexcept
    {
      ::operator delete (p);
    }

    inline void shared_base::
    operator delete (void* p) noexcept
    {
      ::operator delete (p);
    }
  }
}
