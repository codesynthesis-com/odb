// file      : odb/mysql/query.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_QUERY_HXX
#define ODB_MYSQL_QUERY_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/forward.hxx> // odb::query_column
#include <odb/query.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/traits.hxx>
#include <odb/mysql/enum.hxx>
#include <odb/mysql/binding.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/shared-ptr.hxx>

#include <odb/mysql/details/export.hxx>
#include <odb/mysql/details/conversion.hxx>

namespace odb
{
  namespace mysql
  {
    template <typename T>
    struct val_bind
    {
      typedef const T& type;

      explicit
      val_bind (type v): val (v) {}

      type val;
    };

    template <typename T, std::size_t N>
    struct val_bind<T[N]>
    {
      typedef const T* type;

      explicit
      val_bind (type v): val (v) {}

      type val;
    };

    template <typename T>
    struct ref_bind
    {
      typedef const T& type;

      explicit
      ref_bind (type r): ref (r) {}

      const void*
      ptr () const {return &ref;}

      type ref;
    };

    template <typename T, std::size_t N>
    struct ref_bind<T[N]>
    {
      typedef const T* type;

      explicit
      ref_bind (type r): ref (r) {}

      // Allow implicit conversion from decayed ref_bind's.
      //
      ref_bind (ref_bind<T*> r): ref (r.ref) {}
      ref_bind (ref_bind<const T*> r): ref (r.ref) {}

      const void*
      ptr () const {return ref;}

      type ref;
    };

    template <typename T, database_type_id ID>
    struct val_bind_typed: val_bind<T>
    {
      explicit
      val_bind_typed (typename val_bind<T>::type v): val_bind<T> (v) {}
    };

    template <typename T, database_type_id ID>
    struct ref_bind_typed: ref_bind<T>
    {
      explicit
      ref_bind_typed (typename ref_bind<T>::type r): ref_bind<T> (r) {}
    };

    struct LIBODB_MYSQL_EXPORT query_param: details::shared_base
    {
      virtual
      ~query_param ();

      bool
      reference () const
      {
        return value_ != 0;
      }

      virtual bool
      init () = 0;

      virtual void
      bind (MYSQL_BIND*) = 0;

    protected:
      query_param (const void* value) : value_ (value) {}

    protected:
      const void* value_;
    };

    //
    //
    template <typename T, database_type_id ID>
    struct query_column;

    class LIBODB_MYSQL_EXPORT query_base
    {
    public:
      struct clause_part
      {
        enum kind_type
        {
          kind_column,
          kind_param,
          kind_native,
          kind_bool
        };

        clause_part (kind_type k): kind (k), bool_part (false) {}
        clause_part (kind_type k, const std::string& p)
            : kind (k), part (p), bool_part (false) {}
        clause_part (bool p): kind (kind_bool), bool_part (p) {}

        kind_type kind;
        std::string part; // If kind is param, then part is conversion expr.
        bool bool_part;
      };

      query_base ()
        : binding_ (0, 0)
      {
      }

      // True or false literal.
      //
      explicit
      query_base (bool v)
        : binding_ (0, 0)
      {
        append (v);
      }

      explicit
      query_base (const char* native)
        : binding_ (0, 0)
      {
        clause_.push_back (clause_part (clause_part::kind_native, native));
      }

      explicit
      query_base (const std::string& native)
        : binding_ (0, 0)
      {
        clause_.push_back (clause_part (clause_part::kind_native, native));
      }

      query_base (const char* table, const char* column)
        : binding_ (0, 0)
      {
        append (table, column);
      }

      template <typename T>
      explicit
      query_base (val_bind<T> v)
        : binding_ (0, 0)
      {
        *this += v;
      }

      template <typename T, database_type_id ID>
      explicit
      query_base (val_bind_typed<T, ID> v)
        : binding_ (0, 0)
      {
        *this += v;
      }

      template <typename T>
      explicit
      query_base (ref_bind<T> r)
        : binding_ (0, 0)
      {
        *this += r;
      }

      template <typename T, database_type_id ID>
      explicit
      query_base (ref_bind_typed<T, ID> r)
        : binding_ (0, 0)
      {
        *this += r;
      }

      template <database_type_id ID>
      query_base (const query_column<bool, ID>&);

      // Translate common query representation to MySQL native. Defined
      // in query-dynamic.cxx
      //
      query_base (const odb::query_base&);

      // Copy c-tor and assignment.
      //
      query_base (const query_base&);

      query_base&
      operator= (const query_base&);

    public:
      std::string
      clause () const;

      const char*
      clause_prefix () const;

      // Initialize the by-reference parameters from bound variables.
      //
      void
      init_parameters () const;

      binding&
      parameters_binding () const;

    public:
      bool
      empty () const
      {
        return clause_.empty ();
      }

      static const query_base true_expr;

      bool
      const_true () const
      {
        return clause_.size () == 1 &&
          clause_.front ().kind == clause_part::kind_bool &&
          clause_.front ().bool_part;
      }

      void
      optimize ();

    public:
      template <typename T>
      static val_bind<T>
      _val (const T& x)
      {
        return val_bind<T> (x);
      }

      template <database_type_id ID, typename T>
      static val_bind_typed<T, ID>
      _val (const T& x)
      {
        return val_bind_typed<T, ID> (x);
      }

      template <typename T>
      static ref_bind<T>
      _ref (const T& x)
      {
        return ref_bind<T> (x);
      }

      template <database_type_id ID, typename T>
      static ref_bind_typed<T, ID>
      _ref (const T& x)
      {
        return ref_bind_typed<T, ID> (x);
      }

      // Some compilers (notably VC++), when deducing const T& from const
      // array do not strip const from the array type. As a result, in the
      // above signatures we get, for example, T = const char[4] instead
      // of T = char[4], which is what we want. So to "fix" such compilers,
      // we will have to provide the following specializations of the above
      // functions.
      //
      template <typename T, std::size_t N>
      static val_bind<T[N]>
      _val (const T (&x) [N])
      {
        return val_bind<T[N]> (x);
      }

      template <database_type_id ID, typename T, std::size_t N>
      static val_bind_typed<T[N], ID>
      _val (const T (&x) [N])
      {
        return val_bind_typed<T[N], ID> (x);
      }

      template <typename T, std::size_t N>
      static ref_bind<T[N]>
      _ref (const T (&x) [N])
      {
        return ref_bind<T[N]> (x);
      }

      template <database_type_id ID, typename T, std::size_t N>
      static ref_bind_typed<T[N], ID>
      _ref (const T (&x) [N])
      {
        return ref_bind_typed<T[N], ID> (x);
      }

    public:
      query_base&
      operator+= (const query_base&);

      query_base&
      operator+= (const std::string& q)
      {
        append (q);
        return *this;
      }

      template <typename T>
      query_base&
      operator+= (val_bind<T> v)
      {
        append<T, type_traits<T>::db_type_id> (
          v, details::conversion<T>::to ());
        return *this;
      }

      template <typename T, database_type_id ID>
      query_base&
      operator+= (val_bind_typed<T, ID> v)
      {
        // We are not using default type_traits so no default conversion
        // either.
        //
        append<T, ID> (v, 0);
        return *this;
      }

      template <typename T>
      query_base&
      operator+= (ref_bind<T> r)
      {
        append<T, type_traits<T>::db_type_id> (
          r, details::conversion<T>::to ());
        return *this;
      }

      template <typename T, database_type_id ID>
      query_base&
      operator+= (ref_bind_typed<T, ID> r)
      {
        // We are not using default type_traits so no default conversion
        // either.
        //
        append<T, ID> (r, 0);
        return *this;
      }

      // Implementation details.
      //
    public:
      template <typename T, database_type_id ID>
      void
      append (val_bind<T>, const char* conv);

      template <typename T, database_type_id ID>
      void
      append (ref_bind<T>, const char* conv);

      void
      append (details::shared_ptr<query_param>, const char* conv);

      void
      append (bool v)
      {
        clause_.push_back (clause_part (v));
      }

      void
      append (const std::string& native);

      void
      append (const char* native) // Clashes with append(bool).
      {
        append (std::string (native));
      }

      void
      append (const char* table, const char* column);

    private:
      typedef std::vector<clause_part> clause_type;
      typedef std::vector<details::shared_ptr<query_param> > parameters_type;

      clause_type clause_;
      parameters_type parameters_;
      mutable std::vector<MYSQL_BIND> bind_;
      mutable binding binding_;
    };

    inline query_base
    operator+ (const query_base& x, const query_base& y)
    {
      query_base r (x);
      r += y;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (const query_base& q, val_bind<T> b)
    {
      query_base r (q);
      r += b;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (val_bind<T> b, const query_base& q)
    {
      query_base r;
      r += b;
      r += q;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const query_base& q, val_bind_typed<T, ID> b)
    {
      query_base r (q);
      r += b;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (val_bind_typed<T, ID> b, const query_base& q)
    {
      query_base r;
      r += b;
      r += q;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (const query_base& q, ref_bind<T> b)
    {
      query_base r (q);
      r += b;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (ref_bind<T> b, const query_base& q)
    {
      query_base r;
      r += b;
      r += q;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const query_base& q, ref_bind_typed<T, ID> b)
    {
      query_base r (q);
      r += b;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (ref_bind_typed<T, ID> b, const query_base& q)
    {
      query_base r;
      r += b;
      r += q;
      return r;
    }

    inline query_base
    operator+ (const query_base& q, const std::string& s)
    {
      query_base r (q);
      r += s;
      return r;
    }

    inline query_base
    operator+ (const std::string& s, const query_base& q)
    {
      query_base r (s);
      r += q;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (const std::string& s, val_bind<T> b)
    {
      query_base r (s);
      r += b;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (val_bind<T> b, const std::string& s)
    {
      query_base r;
      r += b;
      r += s;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const std::string& s, val_bind_typed<T, ID> b)
    {
      query_base r (s);
      r += b;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (val_bind_typed<T, ID> b, const std::string& s)
    {
      query_base r;
      r += b;
      r += s;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (const std::string& s, ref_bind<T> b)
    {
      query_base r (s);
      r += b;
      return r;
    }

    template <typename T>
    inline query_base
    operator+ (ref_bind<T> b, const std::string& s)
    {
      query_base r;
      r += b;
      r += s;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const std::string& s, ref_bind_typed<T, ID> b)
    {
      query_base r (s);
      r += b;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (ref_bind_typed<T, ID> b, const std::string& s)
    {
      query_base r;
      r += b;
      r += s;
      return r;
    }

    LIBODB_MYSQL_EXPORT query_base
    operator&& (const query_base& x, const query_base& y);

    LIBODB_MYSQL_EXPORT query_base
    operator|| (const query_base& x, const query_base& y);

    LIBODB_MYSQL_EXPORT query_base
    operator! (const query_base& x);

    // query_column
    //
    struct LIBODB_MYSQL_EXPORT query_column_base
    {
      // Note that we keep shallow copies of the table, column, and conversion
      // expression. The latter can be NULL.
      //
      query_column_base (const char* table,
                         const char* column,
                         const char* conv)
          : table_ (table), column_ (column), conversion_ (conv)
      {
      }

      const char*
      table () const
      {
        return table_;
      }

      const char*
      column () const
      {
        return column_;
      }

      // Can be NULL.
      //
      const char*
      conversion () const
      {
        return conversion_;
      }

    protected:
      const char* table_;
      const char* column_;
      const char* conversion_;
    };

    template <typename T, database_type_id ID>
    struct query_column: query_column_base
    {
      typedef typename decay_traits<T>::type decayed_type;

      // Note that we keep shallow copies of the table, column, and conversion
      // expression. The latter can be NULL.
      //
      query_column (const char* table, const char* column, const char* conv)
          : query_column_base (table, column, conv) {}

      // Implementation is in query-dynamic.ixx.
      //
      query_column (odb::query_column<T>&,
                    const char* table, const char* column, const char* conv);

      // is_null, is_not_null
      //
    public:
      query_base
      is_null () const
      {
        query_base q (table_, column_);
        q += "IS NULL";
        return q;
      }

      query_base
      is_not_null () const
      {
        query_base q (table_, column_);
        q += "IS NOT NULL";
        return q;
      }

      // in
      //
    public:
      query_base
      in (decayed_type, decayed_type) const;

      query_base
      in (decayed_type, decayed_type, decayed_type) const;

      query_base
      in (decayed_type, decayed_type, decayed_type, decayed_type) const;

      query_base
      in (decayed_type, decayed_type, decayed_type, decayed_type,
          decayed_type) const;

      template <typename I>
      query_base
      in_range (I begin, I end) const;

      // like
      //
    public:
      query_base
      like (decayed_type pattern) const
      {
        return like (val_bind<T> (pattern));
      }

      query_base
      like (val_bind<T> pattern) const;

      template <typename T2>
      query_base
      like (val_bind<T2> pattern) const
      {
        return like (val_bind<T> (decayed_type (pattern.val)));
      }

      query_base
      like (ref_bind<T> pattern) const;

      query_base
      like (decayed_type pattern, decayed_type escape) const
      {
        return like (val_bind<T> (pattern), escape);
      }

      query_base
      like (val_bind<T> pattern, decayed_type escape) const;

      template <typename T2>
      query_base
      like (val_bind<T2> pattern, decayed_type escape) const
      {
        return like (val_bind<T> (decayed_type (pattern.val)), escape);
      }

      query_base
      like (ref_bind<T> pattern, decayed_type escape) const;

      // =
      //
    public:
      query_base
      equal (decayed_type v) const
      {
        return equal (val_bind<T> (v));
      }

      query_base
      equal (val_bind<T> v) const
      {
        query_base q (table_, column_);
        q += "=";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      equal (val_bind<T2> v) const
      {
        return equal (val_bind<T> (decayed_type (v.val)));
      }

      query_base
      equal (ref_bind<T> r) const
      {
        query_base q (table_, column_);
        q += "=";
        q.append<T, ID> (r, conversion_);
        return q;
      }

      friend query_base
      operator== (const query_column& c, decayed_type v)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (decayed_type v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (const query_column& c, val_bind<T> v)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (val_bind<T> v, const query_column& c)
      {
        return c.equal (v);
      }

      template <typename T2>
      friend query_base
      operator== (const query_column& c, val_bind<T2> v)
      {
        return c.equal (v);
      }

      template <typename T2>
      friend query_base
      operator== (val_bind<T2> v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (const query_column& c, ref_bind<T> r)
      {
        return c.equal (r);
      }

      friend query_base
      operator== (ref_bind<T> r, const query_column& c)
      {
        return c.equal (r);
      }

      // !=
      //
    public:
      query_base
      unequal (decayed_type v) const
      {
        return unequal (val_bind<T> (v));
      }

      query_base
      unequal (val_bind<T> v) const
      {
        query_base q (table_, column_);
        q += "!=";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      unequal (val_bind<T2> v) const
      {
        return unequal (val_bind<T> (decayed_type (v.val)));
      }

      query_base
      unequal (ref_bind<T> r) const
      {
        query_base q (table_, column_);
        q += "!=";
        q.append<T, ID> (r, conversion_);
        return q;
      }

      friend query_base
      operator!= (const query_column& c, decayed_type v)
      {
        return c.unequal (v);
      }

      friend query_base
      operator!= (decayed_type v, const query_column& c)
      {
        return c.unequal (v);
      }

      friend query_base
      operator!= (const query_column& c, val_bind<T> v)
      {
        return c.unequal (v);
      }

      friend query_base
      operator!= (val_bind<T> v, const query_column& c)
      {
        return c.unequal (v);
      }

      template <typename T2>
      friend query_base
      operator!= (const query_column& c, val_bind<T2> v)
      {
        return c.unequal (v);
      }

      template <typename T2>
      friend query_base
      operator!= (val_bind<T2> v, const query_column& c)
      {
        return c.unequal (v);
      }

      friend query_base
      operator!= (const query_column& c, ref_bind<T> r)
      {
        return c.unequal (r);
      }

      friend query_base
      operator!= (ref_bind<T> r, const query_column& c)
      {
        return c.unequal (r);
      }

      // <
      //
    public:
      query_base
      less (decayed_type v) const
      {
        return less (val_bind<T> (v));
      }

      query_base
      less (val_bind<T> v) const
      {
        query_base q (table_, column_);
        q += "<";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      less (val_bind<T2> v) const
      {
        return less (val_bind<T> (decayed_type (v.val)));
      }

      query_base
      less (ref_bind<T> r) const
      {
        query_base q (table_, column_);
        q += "<";
        q.append<T, ID> (r, conversion_);
        return q;
      }

      friend query_base
      operator< (const query_column& c, decayed_type v)
      {
        return c.less (v);
      }

      friend query_base
      operator< (decayed_type v, const query_column& c)
      {
        return c.greater (v);
      }

      friend query_base
      operator< (const query_column& c, val_bind<T> v)
      {
        return c.less (v);
      }

      friend query_base
      operator< (val_bind<T> v, const query_column& c)
      {
        return c.greater (v);
      }

      template <typename T2>
      friend query_base
      operator< (const query_column& c, val_bind<T2> v)
      {
        return c.less (v);
      }

      template <typename T2>
      friend query_base
      operator< (val_bind<T2> v, const query_column& c)
      {
        return c.greater (v);
      }

      friend query_base
      operator< (const query_column& c, ref_bind<T> r)
      {
        return c.less (r);
      }

      friend query_base
      operator< (ref_bind<T> r, const query_column& c)
      {
        return c.greater (r);
      }

      // >
      //
    public:
      query_base
      greater (decayed_type v) const
      {
        return greater (val_bind<T> (v));
      }

      query_base
      greater (val_bind<T> v) const
      {
        query_base q (table_, column_);
        q += ">";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      greater (val_bind<T2> v) const
      {
        return greater (val_bind<T> (decayed_type (v.val)));
      }

      query_base
      greater (ref_bind<T> r) const
      {
        query_base q (table_, column_);
        q += ">";
        q.append<T, ID> (r, conversion_);
        return q;
      }

      friend query_base
      operator> (const query_column& c, decayed_type v)
      {
        return c.greater (v);
      }

      friend query_base
      operator> (decayed_type v, const query_column& c)
      {
        return c.less (v);
      }

      friend query_base
      operator> (const query_column& c, val_bind<T> v)
      {
        return c.greater (v);
      }

      friend query_base
      operator> (val_bind<T> v, const query_column& c)
      {
        return c.less (v);
      }

      template <typename T2>
      friend query_base
      operator> (const query_column& c, val_bind<T2> v)
      {
        return c.greater (v);
      }

      template <typename T2>
      friend query_base
      operator> (val_bind<T2> v, const query_column& c)
      {
        return c.less (v);
      }

      friend query_base
      operator> (const query_column& c, ref_bind<T> r)
      {
        return c.greater (r);
      }

      friend query_base
      operator> (ref_bind<T> r, const query_column& c)
      {
        return c.less (r);
      }

      // <=
      //
    public:
      query_base
      less_equal (decayed_type v) const
      {
        return less_equal (val_bind<T> (v));
      }

      query_base
      less_equal (val_bind<T> v) const
      {
        query_base q (table_, column_);
        q += "<=";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      less_equal (val_bind<T2> v) const
      {
        return less_equal (val_bind<T> (decayed_type (v.val)));
      }

      query_base
      less_equal (ref_bind<T> r) const
      {
        query_base q (table_, column_);
        q += "<=";
        q.append<T, ID> (r, conversion_);
        return q;
      }

      friend query_base
      operator<= (const query_column& c, decayed_type v)
      {
        return c.less_equal (v);
      }

      friend query_base
      operator<= (decayed_type v, const query_column& c)
      {
        return c.greater_equal (v);
      }

      friend query_base
      operator<= (const query_column& c, val_bind<T> v)
      {
        return c.less_equal (v);
      }

      friend query_base
      operator<= (val_bind<T> v, const query_column& c)
      {
        return c.greater_equal (v);
      }

      template <typename T2>
      friend query_base
      operator<= (const query_column& c, val_bind<T2> v)
      {
        return c.less_equal (v);
      }

      template <typename T2>
      friend query_base
      operator<= (val_bind<T2> v, const query_column& c)
      {
        return c.greater_equal (v);
      }

      friend query_base
      operator<= (const query_column& c, ref_bind<T> r)
      {
        return c.less_equal (r);
      }

      friend query_base
      operator<= (ref_bind<T> r, const query_column& c)
      {
        return c.greater_equal (r);
      }

      // >=
      //
    public:
      query_base
      greater_equal (decayed_type v) const
      {
        return greater_equal (val_bind<T> (v));
      }

      query_base
      greater_equal (val_bind<T> v) const
      {
        query_base q (table_, column_);
        q += ">=";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      greater_equal (val_bind<T2> v) const
      {
        return greater_equal (val_bind<T> (decayed_type (v.val)));
      }

      query_base
      greater_equal (ref_bind<T> r) const
      {
        query_base q (table_, column_);
        q += ">=";
        q.append<T, ID> (r, conversion_);
        return q;
      }

      friend query_base
      operator>= (const query_column& c, decayed_type v)
      {
        return c.greater_equal (v);
      }

      friend query_base
      operator>= (decayed_type v, const query_column& c)
      {
        return c.less_equal (v);
      }

      friend query_base
      operator>= (const query_column& c, val_bind<T> v)
      {
        return c.greater_equal (v);
      }

      friend query_base
      operator>= (val_bind<T> v, const query_column& c)
      {
        return c.less_equal (v);
      }

      template <typename T2>
      friend query_base
      operator>= (const query_column& c, val_bind<T2> v)
      {
        return c.greater_equal (v);
      }

      template <typename T2>
      friend query_base
      operator>= (val_bind<T2> v, const query_column& c)
      {
        return c.less_equal (v);
      }

      friend query_base
      operator>= (const query_column& c, ref_bind<T> r)
      {
        return c.greater_equal (r);
      }

      friend query_base
      operator>= (ref_bind<T> r, const query_column& c)
      {
        return c.less_equal (r);
      }

      // Column comparison.
      //
    public:
      template <typename T2, database_type_id ID2>
      query_base
      operator== (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (decay_traits<T>::instance () ==
                        decay_traits<T2>::instance ()));

        query_base q (table_, column_);
        q += "=";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query_base
      operator!= (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (decay_traits<T>::instance () !=
                        decay_traits<T2>::instance ()));

        query_base q (table_, column_);
        q += "!=";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query_base
      operator< (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (decay_traits<T>::instance () <
                        decay_traits<T2>::instance ()));

        query_base q (table_, column_);
        q += "<";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query_base
      operator> (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (decay_traits<T>::instance () >
                        decay_traits<T2>::instance ()));

        query_base q (table_, column_);
        q += ">";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query_base
      operator<= (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (decay_traits<T>::instance () <=
                        decay_traits<T2>::instance ()));

        query_base q (table_, column_);
        q += "<=";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query_base
      operator>= (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (decay_traits<T>::instance () >=
                        decay_traits<T2>::instance ()));

        query_base q (table_, column_);
        q += ">=";
        q.append (c.table (), c.column ());
        return q;
      }
    };

    // Provide operator+() for using columns to construct native
    // query fragments (e.g., ORDER BY).
    //
    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const query_column<T, ID>& c, const std::string& s)
    {
      query_base q (c.table (), c.column ());
      q += s;
      return q;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const std::string& s, const query_column<T, ID>& c)
    {
      query_base q (s);
      q.append (c.table (), c.column ());
      return q;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const query_column<T, ID>& c, const query_base& q)
    {
      query_base r (c.table (), c.column ());
      r += q;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query_base
    operator+ (const query_base& q, const query_column<T, ID>& c)
    {
      query_base r (q);
      r.append (c.table (), c.column ());
      return r;
    }

    //
    //
    template <typename T, database_type_id>
    struct query_param_impl;

    // TINY
    //
    template <typename T>
    struct query_param_impl<T, id_tiny>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_TINY;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_tiny>::set_image (image_, is_null, v);
      }

    private:
      signed char image_;
    };

    template <typename T>
    struct query_param_impl<T, id_utiny>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_TINY;
        b->is_unsigned = true;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_utiny>::set_image (image_, is_null, v);
      }

    private:
      unsigned char image_;
    };

    // SHORT
    //
    template <typename T>
    struct query_param_impl<T, id_short>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_SHORT;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_short>::set_image (image_, is_null, v);
      }

    private:
      short image_;
    };

    template <typename T>
    struct query_param_impl<T, id_ushort>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_SHORT;
        b->is_unsigned = true;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_ushort>::set_image (image_, is_null, v);
      }

    private:
      unsigned short image_;
    };

    // LONG
    //
    template <typename T>
    struct query_param_impl<T, id_long>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_LONG;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_long>::set_image (image_, is_null, v);
      }

    private:
      int image_;
    };

    template <typename T>
    struct query_param_impl<T, id_ulong>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_LONG;
        b->is_unsigned = true;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_ulong>::set_image (image_, is_null, v);
      }

    private:
      unsigned int image_;
    };

    // LONGLONG
    //
    template <typename T>
    struct query_param_impl<T, id_longlong>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_LONGLONG;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_longlong>::set_image (image_, is_null, v);
      }

    private:
      long long image_;
    };

    template <typename T>
    struct query_param_impl<T, id_ulonglong>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_LONGLONG;
        b->is_unsigned = true;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_ulonglong>::set_image (image_, is_null, v);
      }

    private:
      unsigned long long image_;
    };

    // FLOAT
    //
    template <typename T>
    struct query_param_impl<T, id_float>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_FLOAT;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_float>::set_image (image_, is_null, v);
      }

    private:
      float image_;
    };

    // DOUBLE
    //
    template <typename T>
    struct query_param_impl<T, id_double>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_DOUBLE;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_double>::set_image (image_, is_null, v);
      }

    private:
      double image_;
    };

    // DECIMAL
    //
    template <typename T>
    struct query_param_impl<T, id_decimal>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_NEWDECIMAL;
        b->buffer = buffer_.data ();
        b->buffer_length = static_cast<unsigned long> (buffer_.capacity ());
        b->length = &size_;
      }

    private:
      bool
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_decimal>::set_image (buffer_, size, is_null, v);
        size_ = static_cast<unsigned long> (size);
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      unsigned long size_;
    };

    // DATE
    //
    template <typename T>
    struct query_param_impl<T, id_date>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_DATE;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_date>::set_image (image_, is_null, v);
      }

    private:
      MYSQL_TIME image_;
    };

    // TIME
    //
    template <typename T>
    struct query_param_impl<T, id_time>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_TIME;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_time>::set_image (image_, is_null, v);
      }

    private:
      MYSQL_TIME image_;
    };

    // DATETIME
    //
    template <typename T>
    struct query_param_impl<T, id_datetime>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_DATETIME;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_datetime>::set_image (image_, is_null, v);
      }

    private:
      MYSQL_TIME image_;
    };

    // TIMESTAMP
    //
    template <typename T>
    struct query_param_impl<T, id_timestamp>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_TIMESTAMP;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_timestamp>::set_image (image_, is_null, v);
      }

    private:
      MYSQL_TIME image_;
    };

    // YEAR
    //
    template <typename T>
    struct query_param_impl<T, id_year>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_SHORT;
        b->is_unsigned = false;
        b->buffer = &image_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_year>::set_image (image_, is_null, v);
      }

    private:
      short image_;
    };

    // STRING
    //
    template <typename T>
    struct query_param_impl<T, id_string>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_STRING;
        b->buffer = buffer_.data ();
        b->buffer_length = static_cast<unsigned long> (buffer_.capacity ());
        b->length = &size_;
      }

    private:
      bool
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_string>::set_image (buffer_, size, is_null, v);
        size_ = static_cast<unsigned long> (size);
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      unsigned long size_;
    };

    // BLOB
    //
    template <typename T>
    struct query_param_impl<T, id_blob>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_BLOB;
        b->buffer = buffer_.data ();
        b->buffer_length = static_cast<unsigned long> (buffer_.capacity ());
        b->length = &size_;
      }

    private:
      bool
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_blob>::set_image (buffer_, size, is_null, v);
        size_ = static_cast<unsigned long> (size);
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      unsigned long size_;
    };

    // BIT
    //
    template <typename T>
    struct query_param_impl<T, id_bit>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_BLOB;
        b->buffer = buffer_;
        b->buffer_length = static_cast<unsigned long> (sizeof (buffer_));
        b->length = &size_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0);
        value_traits<T, id_bit>::set_image (
          buffer_, sizeof (buffer_), size, is_null, v);
        size_ = static_cast<unsigned long> (size);
      }

    private:
      // Max 64 bit.
      //
      unsigned char buffer_[8];
      unsigned long size_;
    };

    // ENUM
    //
    // The image type can be either integer or string.
    //
    template <typename T>
    struct query_param_impl<T, id_enum>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        enum_traits::bind (*b, image_, size_, 0);
      }

    private:
      bool
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        return enum_traits::set_image (image_, size_, is_null, v);
      }

    private:
      typename value_traits<T, id_enum>::image_type image_;
      unsigned long size_; // Keep size in case it is a string.
    };

    // SET
    //
    template <typename T>
    struct query_param_impl<T, id_set>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (r.ptr ()) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (MYSQL_BIND* b)
      {
        b->buffer_type = MYSQL_TYPE_STRING;
        b->buffer = buffer_.data ();
        b->buffer_length = static_cast<unsigned long> (buffer_.capacity ());
        b->length = &size_;
      }

    private:
      bool
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_set>::set_image (buffer_, size, is_null, v);
        size_ = static_cast<unsigned long> (size);
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      unsigned long size_;
    };
  }
}

// odb::mysql::query and odb::query specialization for MySQL.
//
namespace odb
{
  namespace mysql
  {
    template <typename T>
    class query: public query_base,
                 public query_selector<T, id_mysql>::columns_type
    {
    public:
      // We don't define any typedefs here since they may clash with
      // column names defined by our base type.
      //

      query ()
      {
      }

      explicit
      query (bool v)
          : query_base (v)
      {
      }

      explicit
      query (const char* q)
          : query_base (q)
      {
      }

      explicit
      query (const std::string& q)
          : query_base (q)
      {
      }

      template <typename T2>
      explicit
      query (val_bind<T2> v)
          : query_base (v)
      {
      }

      template <typename T2>
      explicit
      query (ref_bind<T2> r)
          : query_base (r)
      {
      }

      query (const query_base& q)
          : query_base (q)
      {
      }

      template <database_type_id ID>
      query (const query_column<bool, ID>& qc)
          : query_base (qc)
      {
      }

      query (const odb::query_base& q)
          : query_base (q)
      {
      }
    };

    namespace core
    {
      using mysql::query;
    }
  }

  // Derive odb::query from odb::mysql::query so that it can be
  // implicitly converted in mysql::database::query() calls.
  //
  template <typename T>
  class query<T, mysql::query_base>: public mysql::query<T>
  {
  public:
    // We don't define any typedefs here since they may clash with
    // column names defined by our base type.
    //

    query ()
    {
    }

    explicit
    query (bool v)
        : mysql::query<T> (v)
    {
    }

    explicit
    query (const char* q)
        : mysql::query<T> (q)
    {
    }

    explicit
    query (const std::string& q)
        : mysql::query<T> (q)
    {
    }

    template <typename T2>
    explicit
    query (mysql::val_bind<T2> v)
        : mysql::query<T> (v)
    {
    }

    template <typename T2>
    explicit
    query (mysql::ref_bind<T2> r)
        : mysql::query<T> (r)
    {
    }

    query (const mysql::query_base& q)
        : mysql::query<T> (q)
    {
    }

    template <mysql::database_type_id ID>
    query (const mysql::query_column<bool, ID>& qc)
        : mysql::query<T> (qc)
    {
    }
  };
}

#include <odb/mysql/query.ixx>
#include <odb/mysql/query.txx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_QUERY_HXX
