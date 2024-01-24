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
#ifdef ODB_CXX11
      counter_.fetch_add (1, std::memory_order_relaxed);
#else
      ++counter_;
#endif
    }

    inline bool shared_base::
    _dec_ref ()
    {
      // While there are ways to avoid acquire (which is unnecessary except
      // when the counter drops to zero), for our use-cases we'd rather keep
      // it simple.
      //
      return
#ifdef ODB_CXX11
        counter_.fetch_sub (1, std::memory_order_acq_rel) == 1
#else
        --counter_ == 0
#endif
        ? callback_ == 0 || callback_->zero_counter (callback_->arg)
        : false;
    }

    inline std::size_t shared_base::
    _ref_count () const
    {
#ifdef ODB_CXX11
      return counter_.load (std::memory_order_relaxed);
#else
      return counter_;
#endif
    }

#ifdef ODB_CXX11
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
#else
    inline void* shared_base::
    operator new (std::size_t n) throw (std::bad_alloc)
    {
      return ::operator new (n);
    }

    inline void* shared_base::
    operator new (std::size_t n, share) throw (std::bad_alloc)
    {
      return ::operator new (n);
    }
#endif

    inline void shared_base::
    operator delete (void* p, share) ODB_NOTHROW_NOEXCEPT
    {
      ::operator delete (p);
    }

    inline void shared_base::
    operator delete (void* p) ODB_NOTHROW_NOEXCEPT
    {
      ::operator delete (p);
    }
  }
}
