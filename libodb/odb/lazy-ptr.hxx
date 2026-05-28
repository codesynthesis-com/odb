// file      : odb/lazy-ptr.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_LAZY_PTR_HXX
#define ODB_LAZY_PTR_HXX

#include <odb/pre.hxx>

#include <memory>  // std::unique_ptr, std::shared_ptr/weak_ptr
#include <utility> // std::move

#include <odb/forward.hxx>        // odb::core, odb::database
#include <odb/traits.hxx>
#include <odb/lazy-ptr-impl.hxx>

namespace odb
{
  // Raw pointer lazy version.
  //
  template <class T>
  class lazy_ptr
  {
    // Pointer interface.
    //
  public:
    typedef T element_type;

    lazy_ptr ();
    template <class Y> lazy_ptr (Y*);

    lazy_ptr (const lazy_ptr&);
    template <class Y> lazy_ptr (const lazy_ptr<Y>&);

    lazy_ptr& operator= (const lazy_ptr&);
    template <class Y> lazy_ptr& operator= (Y*);
    template <class Y> lazy_ptr& operator= (const lazy_ptr<Y>&);

    void swap (lazy_ptr&);
    void reset ();
    template <class Y> void reset (Y*);

    T& operator* () const;
    T* operator-> () const;
    T* get () const;

    typedef T* lazy_ptr::*unspecified_bool_type;
    operator unspecified_bool_type () const
    {
      return (p_ || i_) ? &lazy_ptr::p_ : 0;
    }

    // Lazy loading interface.
    //
  public:
    typedef odb::database database_type;

    //  NULL      loaded()
    //
    //  true       true      NULL pointer to transient object
    //  false      true      valid pointer to persistent object
    //  true       false     unloaded pointer to persistent object
    //  false      false     valid pointer to transient object
    //
    bool loaded () const;

    T* load () const;

    // Unload the pointer. For transient objects this function is
    // equivalent to reset().
    //
    void unload () const;

    // Get the underlying eager pointer. If this is an unloaded pointer
    // to a persistent object, then the returned pointer will be NULL.
    //
    T* get_eager () const;

    template <class DB, class ID> lazy_ptr (DB&, const ID&);
    template <class DB, class Y> lazy_ptr (DB&, Y*);

    template <class DB, class ID> void reset (DB&, const ID&);
    template <class DB, class Y> void reset (DB&, Y*);

    template <class O = T>
    typename object_traits<O>::id_type object_id () const;

    database_type& database () const;

    // Helpers.
    //
  public:
    template <class Y> bool equal (const lazy_ptr<Y>&) const;

  private:
    template <class Y> friend class lazy_ptr;

    mutable T* p_;
    mutable lazy_ptr_impl<T> i_;
  };

  // operator< and operator<< are not provided.
  //
  template <class T, class Y>
  bool operator== (const lazy_ptr<T>&, const lazy_ptr<Y>&);

  template <class T, class Y>
  bool operator!= (const lazy_ptr<T>&, const lazy_ptr<Y>&);

  template <class T> void swap (lazy_ptr<T>&, lazy_ptr<T>&);

  // C++11 std::unique_ptr lazy version.
  //
  template <class T, class D = std::default_delete<T>>
  class lazy_unique_ptr
  {
    // Standard lazy_unique_ptr interface.
    //
  public:
    typedef T* pointer;     // For now assume it is T*.
    typedef T element_type;
    typedef D deleter_type;

    /*constexpr*/ lazy_unique_ptr () noexcept;
    /*constexpr*/ lazy_unique_ptr (std::nullptr_t) noexcept;
    explicit lazy_unique_ptr (pointer) noexcept;

    // For now assume D is non-reference.
    //
    lazy_unique_ptr (pointer, const deleter_type&) noexcept;
    lazy_unique_ptr (pointer, deleter_type&&) noexcept;

    lazy_unique_ptr (lazy_unique_ptr&&) noexcept;
    template <class T1, class D1> lazy_unique_ptr (lazy_unique_ptr<T1, D1>&&) noexcept;
    //template <class T1> lazy_unique_ptr (std::auto_ptr<T1>&&) noexcept;


    lazy_unique_ptr& operator= (std::nullptr_t) noexcept;
    lazy_unique_ptr& operator= (lazy_unique_ptr&&) noexcept;
    template <class T1, class D1> lazy_unique_ptr& operator= (lazy_unique_ptr<T1, D1>&&) noexcept;

    T& operator* () const;
    pointer operator-> () const noexcept;
    pointer get () const noexcept;
    explicit operator bool() const noexcept;

    pointer release () noexcept;
    void reset (pointer = pointer ()) noexcept;
    void swap (lazy_unique_ptr&) noexcept;

    deleter_type& get_deleter () noexcept;
    const deleter_type& get_deleter () const noexcept;

    lazy_unique_ptr (const lazy_unique_ptr&) = delete;
    lazy_unique_ptr& operator= (const lazy_unique_ptr&) = delete;

    // Initialization/assignment from unique_ptr.
    //
  public:
    template <class T1, class D1> lazy_unique_ptr (std::unique_ptr<T1, D1>&&) noexcept;
    template <class T1, class D1> lazy_unique_ptr& operator= (std::unique_ptr<T1, D1>&&) noexcept;

    // Lazy loading interface.
    //
  public:
    typedef odb::database database_type;

    //  NULL      loaded()
    //
    //  true       true      NULL pointer to transient object
    //  false      true      valid pointer to persistent object
    //  true       false     unloaded pointer to persistent object
    //  false      false     valid pointer to transient object
    //
    bool loaded () const;

    std::unique_ptr<T, D>& load () const;

    // Unload the pointer. For transient objects this function is
    // equivalent to reset().
    //
    void unload () const;

    // Get the underlying eager pointer. If this is an unloaded pointer
    // to a persistent object, then the returned pointer will be NULL.
    //
    std::unique_ptr<T, D>& get_eager () const;

    template <class DB, class ID> lazy_unique_ptr (DB&, const ID&);
    template <class DB> lazy_unique_ptr (DB&, pointer);
    template <class DB> lazy_unique_ptr (DB&, pointer, const deleter_type&);
    template <class DB> lazy_unique_ptr (DB&, pointer, deleter_type&&);
    template <class DB, class T1, class D1> lazy_unique_ptr (DB&, std::unique_ptr<T1, D1>&&);
    //template <class DB, class T1> lazy_unique_ptr (DB&, std::auto_ptr<T1>&&);

    template <class DB, class ID> void reset (DB&, const ID&);
    template <class DB> void reset (DB&, pointer);
    template <class DB, class T1, class D1> void reset (DB&, std::unique_ptr<T1, D1>&&);
    //template <class DB, class T1> void reset (DB&, std::auto_ptr<T1>&&);

    template <class O = T>
    typename object_traits<O>::id_type object_id () const;

    database_type& database () const;

    // Helpers.
    //
  public:
    template <class T1, class D1> bool equal (const lazy_unique_ptr<T1, D1>&) const;

  private:
    template <class T1, class D1> friend class lazy_unique_ptr;

    // Note that it is possible to have a situation where p_ is NULL,
    // i_.id is NULL and i_.db is not NULL. This will happen if the
    // unique_ptr reference returned by load() is transferred to
    // another pointer or reset.
    //
    mutable std::unique_ptr<T, D> p_;
    mutable lazy_ptr_impl<T> i_;
  };

  template <class T> void swap (lazy_unique_ptr<T>&, lazy_unique_ptr<T>&) noexcept;

  // operator< and operator<< are not provided.
  //
  template <class T1, class D1, class T2, class D2>
  bool operator== (const lazy_unique_ptr<T1, D1>&, const lazy_unique_ptr<T2, D2>&);

  template <class T1, class D1, class T2, class D2>
  bool operator!= (const lazy_unique_ptr<T1, D1>&, const lazy_unique_ptr<T2, D2>&);

  template <class T, class D>
  bool operator== (const lazy_unique_ptr<T, D>&, std::nullptr_t) noexcept;

  template <class T, class D>
  bool operator== (std::nullptr_t, const lazy_unique_ptr<T, D>&) noexcept;

  template <class T, class D>
  bool operator!= (const lazy_unique_ptr<T, D>&, std::nullptr_t) noexcept;

  template <class T, class D>
  bool operator!= (std::nullptr_t, const lazy_unique_ptr<T, D>&) noexcept;

  // C++11 std::shared_ptr lazy version.
  //
  template <class T>
  class lazy_weak_ptr;

  template <class T>
  class lazy_shared_ptr
  {
    // The standard shared_ptr interface.
    //
  public:
    typedef T element_type;

    /*constexpr*/ lazy_shared_ptr () noexcept;
    /*constexpr*/ lazy_shared_ptr (std::nullptr_t) noexcept;
    template <class Y> explicit lazy_shared_ptr (Y*);
    template <class Y, class D> lazy_shared_ptr (Y*, D);
    template <class Y, class D, class A> lazy_shared_ptr (Y*, D, A);
    template <class D> lazy_shared_ptr (std::nullptr_t, D);
    template <class D, class A> lazy_shared_ptr (std::nullptr_t, D, A);
    template <class Y> lazy_shared_ptr (const lazy_shared_ptr<Y>&, T*) noexcept;

    // Note: noexcept is not specified since it can throw while
    // copy-constructing i_ (see lazy_ptr_impl for details).
    //
    lazy_shared_ptr (const lazy_shared_ptr&);
    template <class Y> lazy_shared_ptr (const lazy_shared_ptr<Y>&);

    lazy_shared_ptr (lazy_shared_ptr&&) noexcept;
    template <class Y> lazy_shared_ptr (lazy_shared_ptr<Y>&&) noexcept;
    template <class Y> explicit lazy_shared_ptr (const lazy_weak_ptr<Y>&);
    //template <class Y> explicit lazy_shared_ptr (std::auto_ptr<Y>&&);
    template <class Y, class D> lazy_shared_ptr (std::unique_ptr<Y, D>&&);

    ~lazy_shared_ptr ();

    // Note: noexcept is not specified since it can throw while copy-assigning
    // i_ (see lazy_ptr_impl for details).
    //
    lazy_shared_ptr& operator= (const lazy_shared_ptr&);
    template <class Y> lazy_shared_ptr& operator= (const lazy_shared_ptr<Y>&);

    lazy_shared_ptr& operator= (lazy_shared_ptr&&) noexcept;
    template <class Y> lazy_shared_ptr& operator= (lazy_shared_ptr<Y>&&) noexcept;
    //template <class Y> lazy_shared_ptr& operator= (std::auto_ptr<Y>&&);
    template <class Y, class D> lazy_shared_ptr& operator= (std::unique_ptr<Y, D>&&);

    void swap (lazy_shared_ptr&) noexcept;
    void reset () noexcept;
    template <class Y> void reset (Y*);
    template <class Y, class D> void reset (Y*, D);
    template <class Y, class D, class A> void reset (Y*, D, A);

    T* get () const noexcept;
    T& operator* () const noexcept;
    T* operator-> () const noexcept;
    long use_count () const noexcept;
    bool unique () const noexcept;
    explicit operator bool () const noexcept;

    // owner_before () is not provded.

    // Initialization/assignment from shared_ptr and weak_ptr.
    //
  public:
    template <class Y> lazy_shared_ptr (const std::shared_ptr<Y>&);
    template <class Y> lazy_shared_ptr (std::shared_ptr<Y>&&);
    template <class Y> explicit lazy_shared_ptr (const std::weak_ptr<Y>&);

    template <class Y> lazy_shared_ptr& operator= (const std::shared_ptr<Y>&);
    template <class Y> lazy_shared_ptr& operator= (std::shared_ptr<Y>&&);

    // Lazy loading interface.
    //
  public:
    typedef odb::database database_type;

    //  NULL      loaded()
    //
    //  true       true      NULL pointer to transient object
    //  false      true      valid pointer to persistent object
    //  true       false     unloaded pointer to persistent object
    //  false      false     valid pointer to transient object
    //
    bool loaded () const;

    std::shared_ptr<T> load () const;

    // Unload the pointer. For transient objects this function is
    // equivalent to reset().
    //
    void unload () const;

    // Get the underlying eager pointer. If this is an unloaded pointer
    // to a persistent object, then the returned pointer will be NULL.
    //
    std::shared_ptr<T> get_eager () const;

    template <class DB, class ID> lazy_shared_ptr (DB&, const ID&);
    template <class DB, class Y> lazy_shared_ptr (DB&, Y*);
    template <class DB, class Y, class D> lazy_shared_ptr (DB&, Y*, D);
    template <class DB, class Y, class D, class A> lazy_shared_ptr (DB&, Y*, D, A);
    //template <class DB, class Y> lazy_shared_ptr (DB&, std::auto_ptr<Y>&&);
    template <class DB, class Y> lazy_shared_ptr (DB&, const std::shared_ptr<Y>&);
    template <class DB, class Y> lazy_shared_ptr (DB&, std::shared_ptr<Y>&&);
    template <class DB, class Y> lazy_shared_ptr (DB&, const std::weak_ptr<Y>&);

    template <class DB, class ID> void reset (DB&, const ID&);
    template <class DB, class Y> void reset (DB&, Y*);
    template <class DB, class Y, class D> void reset (DB&, Y*, D);
    template <class DB, class Y, class D, class A> void reset (DB&, Y*, D, A);
    //template <class DB, class Y> void reset (DB&, std::auto_ptr<Y>&&);
    template <class DB, class Y> void reset (DB&, const std::shared_ptr<Y>&);
    template <class DB, class Y> void reset (DB&, std::shared_ptr<Y>&&);

    template <class O = T>
    typename object_traits<O>::id_type object_id () const;

    database_type& database () const;

    // Helpers.
    //
  public:

    // Note: noexcept is not specified since it can throw while creating
    // object ids for comparison.
    //
    template <class Y> bool equal (const lazy_shared_ptr<Y>&) const;

  private:
    template <class Y> friend class lazy_shared_ptr;
    template <class Y> friend class lazy_weak_ptr;

    // For lazy_weak_ptr::lock().
    //
    lazy_shared_ptr (std::shared_ptr<T>&& p, const lazy_ptr_impl<T>& i)
        : p_ (std::move (p)), i_ (i) {}

  private:
    mutable std::shared_ptr<T> p_;
    mutable lazy_ptr_impl<T> i_;
  };

  template <class T> void swap (lazy_shared_ptr<T>&, lazy_shared_ptr<T>&) noexcept;

  template <class D, class T>
  D* get_deleter (const lazy_shared_ptr<T>&) noexcept;

  // operator< and operator<< are not provided.
  //
  // Note: noexcept is not specified since it can throw while calling
  // lazy_shared_ptr::equal().
  //
  template <class T, class Y>
  bool operator== (const lazy_shared_ptr<T>&, const lazy_shared_ptr<Y>&);
  template <class T, class Y>
  bool operator!= (const lazy_shared_ptr<T>&, const lazy_shared_ptr<Y>&);

  template <class T>
  bool operator== (const lazy_shared_ptr<T>&, std::nullptr_t) noexcept;

  template <class T>
  bool operator== (std::nullptr_t, const lazy_shared_ptr<T>&) noexcept;

  template <class T>
  bool operator!= (const lazy_shared_ptr<T>&, std::nullptr_t) noexcept;

  template <class T>
  bool operator!= (std::nullptr_t, const lazy_shared_ptr<T>&) noexcept;

  // C++11 std::weak_ptr lazy version.
  //
  template <class T>
  class lazy_weak_ptr
  {
    // The standard weak_ptr interface.
    //
  public:
    typedef T element_type;

    /*constexpr*/ lazy_weak_ptr () noexcept;

    // Note: noexcept is not specified since it can throw while
    // copy-constructing i_ (see lazy_ptr_impl for details).
    //
    template <class Y> lazy_weak_ptr (const lazy_shared_ptr<Y>&);
    lazy_weak_ptr (const lazy_weak_ptr&);
    template <class Y> lazy_weak_ptr (const lazy_weak_ptr<Y>&);

    ~lazy_weak_ptr ();

    // Note: noexcept is not specified since it can throw while copy-assigning
    // i_ (see lazy_ptr_impl for details).
    //
    lazy_weak_ptr& operator= (const lazy_weak_ptr&);
    template <class Y> lazy_weak_ptr& operator= (const lazy_weak_ptr<Y>&);
    template <class Y> lazy_weak_ptr& operator= (const lazy_shared_ptr<Y>&);

    void swap (lazy_weak_ptr<T>&) noexcept;
    void reset () noexcept;

    long use_count () const noexcept;
    bool expired () const noexcept;

    // Note: noexcept is not specified since it can throw while copying i_
    // (see lazy_ptr_impl for details).
    //
    lazy_shared_ptr<T> lock () const;

    // owner_before () is not provded.

    // Initialization/assignment from shared_ptr and weak_ptr.
    //
  public:
    template <class Y> lazy_weak_ptr (const std::weak_ptr<Y>&);
    template <class Y> lazy_weak_ptr (const std::shared_ptr<Y>&);

    template <class Y> lazy_weak_ptr& operator= (const std::weak_ptr<Y>&);
    template <class Y> lazy_weak_ptr& operator= (const std::shared_ptr<Y>&);

    // Lazy loading interface.
    //
  public:
    typedef odb::database database_type;

    // expired()  loaded()
    //
    //  true       true      expired pointer to transient object
    //  false      true      valid pointer to persistent object
    //  true       false     expired pointer to persistent object
    //  false      false     valid pointer to transient object
    //
    bool loaded () const;

    // Performs both lock and load.
    //
    std::shared_ptr<T> load () const;

    // Unload the pointer. For transient objects this function is
    // equivalent to reset().
    //
    void unload () const;

    // Get the underlying eager pointer. If this is an unloaded pointer
    // to a persistent object, then the returned pointer will be NULL.
    //
    std::weak_ptr<T> get_eager () const;

    template <class DB, class ID> lazy_weak_ptr (DB&, const ID&);
    template <class DB, class Y> lazy_weak_ptr (DB&, const std::shared_ptr<Y>&);
    template <class DB, class Y> lazy_weak_ptr (DB&, const std::weak_ptr<Y>&);

    template <class DB, class ID> void reset (DB&, const ID&);
    template <class DB, class Y> void reset (DB&, const std::shared_ptr<Y>&);
    template <class DB, class Y> void reset (DB&, const std::weak_ptr<Y>&);

    // The object_id() function can only be called when the object is
    // persistent, or: expired() XOR loaded() (can use != for XOR).
    //
    template <class O = T>
    typename object_traits<O>::id_type object_id () const;

    database_type& database () const;

  private:
    template <class Y> friend class lazy_shared_ptr;
    template <class Y> friend class lazy_weak_ptr;

    mutable std::weak_ptr<T> p_;
    mutable lazy_ptr_impl<T> i_;
  };

  // operator< is not provided.
  //
  template <class T> void swap (lazy_weak_ptr<T>&, lazy_weak_ptr<T>&);

  namespace common
  {
    using odb::lazy_ptr;
    using odb::lazy_unique_ptr;
    using odb::lazy_shared_ptr;
    using odb::lazy_weak_ptr;
  }
}

#include <odb/lazy-ptr.ixx>
#include <odb/lazy-ptr.txx>

#include <odb/lazy-pointer-traits.hxx>

#include <odb/post.hxx>

#endif // ODB_LAZY_PTR_HXX
