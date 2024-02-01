// file      : odb/boost/smart-ptr/lazy-ptr.txx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace boost
  {
    //
    // lazy_shared_ptr
    //

    template <class T>
    template <class Y>
    bool lazy_shared_ptr<T>::
    equal (const lazy_shared_ptr<Y>& r) const
    {
      bool t1 (!p_ == loaded ());
      bool t2 (!r.p_ == r.loaded ());

      // If both are transient, then compare the underlying pointers.
      //
      if (t1 && t2)
        return p_ == r.p_;

      // If one is transient and the other is persistent, then compare
      // the underlying pointers but only if they are non NULL. Note
      // that an unloaded persistent object is always unequal to a
      // transient object.
      //
      if (t1 || t2)
        return p_ == r.p_ && p_;

      // If both objects are persistent, then we compare databases and
      // object ids.
      //
      typedef typename object_traits<T>::object_type object_type1;
      typedef typename object_traits<Y>::object_type object_type2;

      return i_.database () == r.i_.database () &&
        object_id<object_type1> () == r.template object_id<object_type2> ();
    }
  }
}
