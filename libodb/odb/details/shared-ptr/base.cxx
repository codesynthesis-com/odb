// file      : odb/details/shared-ptr/base.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/details/shared-ptr/base.hxx>
#include <odb/details/shared-ptr/exception.hxx>

using std::size_t;

namespace odb
{
  namespace details
  {
    share shared = share (1);
    share exclusive = share (2);

    const char* not_shared::
    what () const ODB_NOTHROW_NOEXCEPT
    {
      return "object is not shared";
    }

    not_shared* not_shared::
    clone () const
    {
      return new not_shared (*this);
    }

    namespace bits
    {
      size_t* locator_common::
      counter (void* x)
      {
        size_t* p (static_cast<size_t*> (x));

        if (*(--p) != 0xDEADBEEF)
          throw not_shared ();

        return --p;
      }
    }
  }
}

void*
#ifdef ODB_CXX11
operator new (size_t n, odb::details::share s)
#else
operator new (size_t n, odb::details::share s) throw (std::bad_alloc)
#endif
{
  if (s == odb::details::shared)
  {
    // Here we need to make sure we don't break the alignment of the
    // returned block. For that we need to know the maximum alignment
    // of this platform. Twice the pointer size is a good guess for
    // most platforms.
    //
    size_t* p = static_cast<size_t*> (operator new (n + 2 * sizeof (size_t)));
    *p++ = 1;          // Initial count.
    *p++ = 0xDEADBEEF; // Signature.
    return p;
  }
  else
    return operator new (n);

}

void
operator delete (void* p, odb::details::share s) ODB_NOTHROW_NOEXCEPT
{
  // This version of operator delete is only called when the c-tor
  // fails. In this case there is no object and we can just free the
  // memory.
  //
  if (s == odb::details::shared)
  {
    size_t* sp = static_cast<size_t*> (p);
    sp -= 2;
    operator delete (sp);
  }
  else
    operator delete (p);
}
