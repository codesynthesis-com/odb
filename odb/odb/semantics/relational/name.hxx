// file      : odb/semantics/relational/name.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_RELATIONAL_NAME_HXX
#define ODB_SEMANTICS_RELATIONAL_NAME_HXX

#include <string>
#include <vector>
#include <iosfwd>

namespace semantics
{
  namespace relational
  {
    typedef std::string uname;

    class qname
    {
    public:
      typedef relational::uname uname_type;

      qname () {}

      explicit
      qname (uname_type const& n) {append (n);}

      template <typename I>
      qname (I begin, I end)
      {
        for (; begin != end; ++begin)
          append (*begin);
      }

      qname&
      operator= (uname_type const& n)
      {
        components_.resize (1);
        components_[0] = n;
        return *this;
      }

      void
      append (uname_type const& n) {components_.push_back (n);}

      void
      append (qname const& n)
      {
        components_.insert (components_.end (),
                            n.components_.begin (),
                            n.components_.end ());
      }

      void
      clear () {components_.clear ();}

      // Append a string to the last component.
      //
      qname&
      operator+= (std::string const& s)
      {
        if (empty ())
          append (s);
        else
          uname () += s;

        return *this;
      }

      friend qname
      operator+ (qname const& n, std::string const& s)
      {
        qname r (n);

        if (r.empty ())
          r.append (s);
        else
          r.uname () += s;

        return r;
      }

      void
      swap (qname& n) {components_.swap (n.components_);}

    public:
      bool
      empty () const {return components_.empty ();}

      bool
      qualified () const {return components_.size () > 1;}

      bool
      fully_qualified () const
      {
        return qualified () && components_.front ().empty ();
      }

    public:
      typedef std::vector<uname_type> components;
      typedef components::const_iterator iterator;

      iterator
      begin () const {return components_.begin ();}

      iterator
      end () const {return components_.end ();}

      uname_type&
      uname () {return components_.back ();}

      uname_type const&
      uname () const {return components_.back ();}

      qname
      qualifier () const
      {
        return empty ()
          ? qname ()
          : qname (components_.begin (), components_.end () - 1);
      }

      std::string
      string () const;

      static qname
      from_string (std::string const&);

    public:
      friend bool
      operator== (qname const& x, qname const& y)
      {
        return x.components_ == y.components_;
      }

      friend bool
      operator!= (qname const& x, qname const& y)
      {
        return x.components_ != y.components_;
      }

      friend bool
      operator< (qname const& x, qname const& y)
      {
        return x.components_ < y.components_;
      }

    private:
      components components_;
    };

    std::ostream&
    operator<< (std::ostream&, qname const&);

    std::istream&
    operator>> (std::istream&, qname&);
  }
}

#endif // ODB_SEMANTICS_RELATIONAL_NAME_HXX
