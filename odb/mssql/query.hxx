// file      : odb/mssql/query.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_QUERY_HXX
#define ODB_MSSQL_QUERY_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/forward.hxx> // odb::query_column
#include <odb/query.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/traits.hxx>
#include <odb/mssql/binding.hxx>
#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/mssql-types.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/shared-ptr.hxx>

#include <odb/mssql/details/export.hxx>
#include <odb/mssql/details/conversion.hxx>

namespace odb
{
  namespace mssql
  {
    // For precision, 0 is invalid so we can use it as a special value
    // to indicate that the precision has not been specified. For scale,
    // however, 0 is a valid value and for some types (e.g., TIME) if
    // the scale is not specified, it defaults to something other than
    // 0. As a result, for scale, the not specific special value will
    // be 0xFFFF (USHORT_MAX).
    //
    template <typename T>
    struct val_bind
    {
      typedef const T& type;

      explicit
      val_bind (type v, unsigned short p = 0, unsigned short s = 0xFFFF)
          : val (v), prec (p), scale (s) {}

      type val;

      unsigned short prec;
      unsigned short scale;
    };

    template <typename T, std::size_t N>
    struct val_bind<T[N]>
    {
      typedef const T* type;

      explicit
      val_bind (type v, unsigned short p = 0, unsigned short s = 0xFFFF)
          : val (v), prec (p), scale (s) {}

      type val;

      unsigned short prec;
      unsigned short scale;
    };

    template <typename T>
    struct ref_bind
    {
      typedef const T& type;

      explicit
      ref_bind (type r, unsigned short p = 0, unsigned short s = 0xFFFF)
          : ref (r), prec (p), scale (s) {}

      const void*
      ptr () const {return &ref;}

      type ref;

      unsigned short prec;
      unsigned short scale;
    };

    template <typename T, std::size_t N>
    struct ref_bind<T[N]>
    {
      typedef const T* type;

      explicit
      ref_bind (type r, unsigned short p = 0, unsigned short s = 0xFFFF)
          : ref (r), prec (p), scale (s) {}

      // Allow implicit conversion from decayed ref_bind's.
      //
      ref_bind (ref_bind<T*> r): ref (r.ref), prec (r.prec), scale (r.scale) {}
      ref_bind (ref_bind<const T*> r)
          : ref (r.ref), prec (r.prec), scale (r.scale) {}

      const void*
      ptr () const {return ref;}

      type ref;

      unsigned short prec;
      unsigned short scale;
    };

    template <typename T, database_type_id ID>
    struct val_bind_typed: val_bind<T>
    {
      explicit
      val_bind_typed (typename val_bind<T>::type v,
                      unsigned short p = 0,
                      unsigned short s = 0xFFFF): val_bind<T> (v, p, s) {}
    };

    template <typename T, database_type_id ID>
    struct ref_bind_typed: ref_bind<T>
    {
      explicit
      ref_bind_typed (typename ref_bind<T>::type r,
                      unsigned short p = 0,
                      unsigned short s = 0xFFFF): ref_bind<T> (r, p, s) {}
    };

    struct LIBODB_MSSQL_EXPORT query_param: details::shared_base
    {
      typedef mssql::bind bind_type;

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
      bind (bind_type*) = 0;

    protected:
      query_param (const void* value)
          : value_ (value)
      {
      }

    protected:
      const void* value_;
    };

    //
    //
    template <typename T, database_type_id ID>
    struct query_column;

    class LIBODB_MSSQL_EXPORT query_base
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

        clause_part (kind_type k): kind (k) {}
        clause_part (kind_type k, const std::string& p): kind (k), part (p) {}
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

      // Translate common query representation to SQL Server native. Defined
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
      _val (const T& x, unsigned short prec = 0, unsigned short scale = 0xFFFF)
      {
        return val_bind<T> (x, prec, scale);
      }

      template <database_type_id ID, typename T>
      static val_bind_typed<T, ID>
      _val (const T& x, unsigned short prec = 0, unsigned short scale = 0xFFFF)
      {
        return val_bind_typed<T, ID> (x, prec, scale);
      }

      template <typename T>
      static ref_bind<T>
      _ref (const T& x, unsigned short prec = 0, unsigned short scale = 0xFFFF)
      {
        return ref_bind<T> (x, prec, scale);
      }

      template <database_type_id ID, typename T>
      static ref_bind_typed<T, ID>
      _ref (const T& x, unsigned short prec = 0, unsigned short scale = 0xFFFF)
      {
        return ref_bind_typed<T, ID> (x, prec, scale);
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
      _val (const T (&x) [N], unsigned short p = 0, unsigned short s = 0xFFFF)
      {
        return val_bind<T[N]> (x, p, s);
      }

      template <database_type_id ID, typename T, std::size_t N>
      static val_bind_typed<T[N], ID>
      _val (const T (&x) [N], unsigned short p = 0, unsigned short s = 0xFFFF)
      {
        return val_bind_typed<T[N], ID> (x, p, s);
      }

      template <typename T, std::size_t N>
      static ref_bind<T[N]>
      _ref (const T (&x) [N], unsigned short p = 0, unsigned short s = 0xFFFF)
      {
        return ref_bind<T[N]> (x, p, s);
      }

      template <database_type_id ID, typename T, std::size_t N>
      static ref_bind_typed<T[N], ID>
      _ref (const T (&x) [N], unsigned short p = 0, unsigned short s = 0xFFFF)
      {
        return ref_bind_typed<T[N], ID> (x, p, s);
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
      mutable std::vector<bind> bind_;
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

    LIBODB_MSSQL_EXPORT query_base
    operator&& (const query_base& x, const query_base& y);

    LIBODB_MSSQL_EXPORT query_base
    operator|| (const query_base& x, const query_base& y);

    LIBODB_MSSQL_EXPORT query_base
    operator! (const query_base& x);

    // query_column
    //
    struct LIBODB_MSSQL_EXPORT query_column_base
    {
      // Note that we keep shallow copies of the table, column, and conversion
      // expression. The latter can be NULL.
      //
      query_column_base (const char* table,
                         const char* column,
                         const char* conv,
                         unsigned short prec,
                         unsigned short scale)
        : table_ (table), column_ (column), conversion_ (conv),
          prec_ (prec), scale_ (scale)
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

      unsigned short
      prec () const
      {
        return prec_;
      }

      unsigned short
      scale () const
      {
        return scale_;
      }

    protected:
      const char* table_;
      const char* column_;
      const char* conversion_;

      unsigned short prec_;
      unsigned short scale_;
    };

    template <typename T, database_type_id ID>
    struct query_column: query_column_base
    {
      typedef typename decay_traits<T>::type decayed_type;

      // Note that we keep shalow copies of the table, column, and conversion
      // expression. The latter can be NULL.
      //
      query_column (const char* table,
                    const char* column,
                    const char* conv,
                    unsigned short prec = 0,
                    unsigned short scale = 0xFFFF)
          : query_column_base (table, column, conv, prec, scale) {}

      // Implementation is in query-dynamic.ixx.
      //
      query_column (odb::query_column<T>&,
                    const char* table,
                    const char* column,
                    const char* conv,
                    unsigned short prec = 0,
                    unsigned short scale = 0xFFFF);

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
        v.prec = prec_;
        v.scale = scale_;

        query_base q (table_, column_);
        q += "=";
        q.append<T, ID> (v, conversion_);
        return q;
      }

      template <typename T2>
      query_base
      equal (val_bind<T2> v) const
      {
        return equal (val_bind<T> (decayed_type (v.val)));;
      }

      query_base
      equal (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

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
        v.prec = prec_;
        v.scale = scale_;

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
        r.prec = prec_;
        r.scale = scale_;

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
        v.prec = prec_;
        v.scale = scale_;

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
        r.prec = prec_;
        r.scale = scale_;

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
        v.prec = prec_;
        v.scale = scale_;

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
        r.prec = prec_;
        r.scale = scale_;

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
        v.prec = prec_;
        v.scale = scale_;

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
        r.prec = prec_;
        r.scale = scale_;

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
        v.prec = prec_;
        v.scale = scale_;

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
        r.prec = prec_;
        r.scale = scale_;

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

    // id_bit.
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
      bind (bind_type* b)
      {
        b->type = bind::bit;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_bit>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      unsigned char image_;
      SQLLEN size_ind_;
    };

    // id_tinyint.
    //
    template <typename T>
    struct query_param_impl<T, id_tinyint>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::tinyint;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_tinyint>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      unsigned char image_;
      SQLLEN size_ind_;
    };

    // id_smallint.
    //
    template <typename T>
    struct query_param_impl<T, id_smallint>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::smallint;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_smallint>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      short image_;
      SQLLEN size_ind_;
    };

    // id_int.
    //
    template <typename T>
    struct query_param_impl<T, id_int>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::int_;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_int>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      int image_;
      SQLLEN size_ind_;
    };

    // id_bigint.
    //
    template <typename T>
    struct query_param_impl<T, id_bigint>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::bigint;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_bigint>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      long long image_;
      SQLLEN size_ind_;
    };

    // id_decimal.
    //
    template <typename T>
    struct query_param_impl<T, id_decimal>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            prec_ (r.prec != 0 ? r.prec : 18), // Default is 18.
            scale_ (r.scale != 0xFFFF ? r.scale : 0) // Default is 0.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            prec_ (v.prec != 0 ? v.prec : 18), // Default is 18.
            scale_ (v.scale)
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::decimal;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
        // Encode precision (p) and scale (s) as (p * 100 + s).
        //
        b->capacity = static_cast<SQLLEN> (prec_ * 100 + scale_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_decimal>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      unsigned short prec_;
      unsigned short scale_;

      decimal image_;
      SQLLEN size_ind_;
    };

    // id_smallmoney.
    //
    template <typename T>
    struct query_param_impl<T, id_smallmoney>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::smallmoney;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_smallmoney>::set_image (image_, is_null, v);
        size_ind_ = 4;
      }

    private:
      smallmoney image_;
      SQLLEN size_ind_;
    };

    // id_money.
    //
    template <typename T>
    struct query_param_impl<T, id_money>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::money;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_money>::set_image (image_, is_null, v);
        size_ind_ = 8;
      }

    private:
      money image_;
      SQLLEN size_ind_;
    };

    // id_float4.
    //
    template <typename T>
    struct query_param_impl<T, id_float4>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            prec_ (r.prec != 0 ? r.prec : 24)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            prec_ (v.prec != 0 ? v.prec : 24)
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::float4;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
        b->capacity = static_cast<SQLLEN> (prec_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_float4>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      unsigned short prec_;
      float image_;
      SQLLEN size_ind_;
    };

    // id_float8.
    //
    template <typename T>
    struct query_param_impl<T, id_float8>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            prec_ (r.prec != 0 ? r.prec : 53)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            prec_ (v.prec != 0 ? v.prec : 53)
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::float8;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
        b->capacity = static_cast<SQLLEN> (prec_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_float8>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      unsigned short prec_;
      double image_;
      SQLLEN size_ind_;
    };

    // id_string.
    //
    template <typename T>
    struct query_param_impl<T, id_string>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            // Default to short data max (1024).
            buf_ (r.prec != 0 ? r.prec : 1024)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            // Default to short data max (1024).
            buf_ (v.prec != 0 ? v.prec : 1024)
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::string;
        b->buffer = buf_.data ();
        b->size_ind = &size_ind_;
        // Extra byte for the null terminator (convention).
        //
        b->capacity = static_cast<SQLLEN> (buf_.capacity () + 1);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0);
        value_traits<T, id_string>::set_image (
          buf_.data (), buf_.capacity (), size, is_null, v);
        size_ind_ = static_cast<SQLLEN> (size);
      }

    private:
      details::buffer buf_;
      SQLLEN size_ind_;
    };

    // id_nstring.
    //
    template <typename T>
    struct query_param_impl<T, id_nstring>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            // Precision is in 2-byte chars. Default to short data max (1024).
            buf_ (r.prec != 0 ? r.prec * 2 : 1024)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            // Precision is in 2-byte chars. Default to short data max (1024).
            buf_ (v.prec != 0 ? v.prec * 2 : 1024)
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::nstring;
        b->buffer = buf_.data ();
        b->size_ind = &size_ind_;
        // Extra two bytes for the null terminator (convention).
        //
        b->capacity = static_cast<SQLLEN> (buf_.capacity () + 2);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0);
        value_traits<T, id_nstring>::set_image (
          reinterpret_cast<ucs2_char*> (buf_.data ()),
          buf_.capacity () / 2,
          size,
          is_null,
          v);
        size_ind_ = static_cast<SQLLEN> (size * 2);
      }

    private:
      details::buffer buf_;
      SQLLEN size_ind_;
    };

    // id_binary.
    //
    template <typename T>
    struct query_param_impl<T, id_binary>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            // Default to short data max (1024).
            buf_ (r.prec != 0 ? r.prec : 1024)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            // Default to short data max (1024).
            buf_ (v.prec != 0 ? v.prec : 1024)
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::binary;
        b->buffer = buf_.data ();
        b->size_ind = &size_ind_;
        b->capacity = static_cast<SQLLEN> (buf_.capacity ());
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        std::size_t size (0);
        value_traits<T, id_binary>::set_image (
          buf_.data (), buf_.capacity (), size, is_null, v);
        size_ind_ = static_cast<SQLLEN> (size);
      }

    private:
      details::buffer buf_;
      SQLLEN size_ind_;
    };

    // long_query_param_impl
    //
    // For long data we cannot assume that the by-value parameter will
    // still be alive when we execute the query (and when the callback
    // will be called to provide the data). So we have to make a private
    // copy of the data and use that when the time comes.
    //
    class LIBODB_MSSQL_EXPORT long_query_param_impl
    {
    protected:
      long_query_param_impl (): buf_ (0) {}

      void
      copy ();

    protected:
      details::buffer buf_;
      std::size_t size_;
      long_callback callback_;
    };

    // id_long_string.
    //
    template <typename T>
    struct query_param_impl<T, id_long_string>: query_param,
                                                long_query_param_impl
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()), prec_ (r.prec)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0), prec_ (v.prec)
      {
        init (v.val);
        copy ();
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        // If this is a by-value parameter then we already have the
        // buffer containing all the data. So instead of using the
        // callback mechanism, bind the buffer directly using the
        // short data approach. SQLLEN  (int on 32-bit platforms)
        // can represent maximum size of 2^31 bytes which happens
        // to be greater than the maximum size of VARCHAR(max) or
        // TEXT (both 2^31-1 bytes).
        //
        if (reference ())
        {
          b->type = bind::long_string;
          b->buffer = &this->long_query_param_impl::callback_;
          b->size_ind = &size_ind_;
          b->capacity = static_cast<SQLLEN> (prec_);
          size_ind_ = SQL_DATA_AT_EXEC;
        }
        else
        {
          b->type = bind::string;
          b->buffer = buf_.data ();
          b->size_ind = &size_ind_;
          // Extra byte for the null terminator (convention).
          //
          b->capacity = static_cast<SQLLEN> (prec_ != 0 ? prec_ + 1 : 0);
          size_ind_ = static_cast<SQLLEN> (size_);
        }
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_long_string>::set_image (
          long_query_param_impl::callback_.callback.param,
          long_query_param_impl::callback_.context.param,
          is_null,
          v);
      }

    private:
      unsigned short prec_;
      SQLLEN size_ind_;
    };

    // id_long_nstring.
    //
    template <typename T>
    struct query_param_impl<T, id_long_nstring>: query_param,
                                                long_query_param_impl
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()), prec_ (r.prec)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0), prec_ (v.prec)
      {
        init (v.val);
        copy ();
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        // If this is a by-value parameter then we already have the
        // buffer containing all the data. So instead of using the
        // callback mechanism, bind the buffer directly using the
        // short data approach. SQLLEN  (int on 32-bit platforms)
        // can represent maximum size of 2^31 bytes which happens
        // to be greater than the maximum size of NVARCHAR(max) or
        // NTEXT (both 2^31-1 bytes).
        //
        if (reference ())
        {
          b->type = bind::long_nstring;
          b->buffer = &this->long_query_param_impl::callback_;
          b->size_ind = &size_ind_;
          b->capacity = static_cast<SQLLEN> (prec_ * 2); // In bytes.
          size_ind_ = SQL_DATA_AT_EXEC;
        }
        else
        {
          b->type = bind::nstring;
          b->buffer = buf_.data ();
          b->size_ind = &size_ind_;
          // In bytes, extra character for the null terminator (convention).
          //
          b->capacity = static_cast<SQLLEN> (prec_ != 0 ? (prec_ + 1) * 2 : 0);
          size_ind_ = static_cast<SQLLEN> (size_);
        }
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_long_nstring>::set_image (
          long_query_param_impl::callback_.callback.param,
          long_query_param_impl::callback_.context.param,
          is_null,
          v);
      }

    private:
      unsigned short prec_;
      SQLLEN size_ind_;
    };

    // id_long_binary.
    //
    template <typename T>
    struct query_param_impl<T, id_long_binary>: query_param,
                                                long_query_param_impl
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()), prec_ (r.prec)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0), prec_ (v.prec)
      {
        init (v.val);
        copy ();
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        // If this is a by-value parameter then we already have the
        // buffer containing all the data. So instead of using the
        // callback mechanism, bind the buffer directly using the
        // short data approach. SQLLEN  (int on 32-bit platforms)
        // can represent maximum size of 2^31 bytes which happens
        // to be greater than the maximum size of VARBINARY(max)
        // or IMAGE (both 2^31-1 bytes).
        //
        if (reference ())
        {
          b->type = bind::long_binary;
          b->buffer = &this->long_query_param_impl::callback_;
          b->size_ind = &size_ind_;
          size_ind_ = SQL_DATA_AT_EXEC;
        }
        else
        {
          b->type = bind::binary;
          b->buffer = buf_.data ();
          b->size_ind = &size_ind_;
          size_ind_ = static_cast<SQLLEN> (size_);
        }

        b->capacity = static_cast<SQLLEN> (prec_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_long_binary>::set_image (
          long_query_param_impl::callback_.callback.param,
          long_query_param_impl::callback_.context.param,
          is_null,
          v);
      }

    private:
      unsigned short prec_;
      SQLLEN size_ind_;
    };

    // id_date.
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
      bind (bind_type* b)
      {
        b->type = bind::date;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_date>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      date image_;
      SQLLEN size_ind_;
    };

    // id_time.
    //
    template <typename T>
    struct query_param_impl<T, id_time>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            scale_ (r.scale != 0xFFFF ? r.scale : 7) // Default is 7.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            scale_ (v.scale != 0xFFFF ? v.scale : 7) // Default is 7.
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::time;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
        b->capacity = static_cast<SQLLEN> (scale_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_time>::set_image (image_, scale_, is_null, v);
        size_ind_ = static_cast<SQLLEN> (sizeof (image_));
      }

    private:
      unsigned short scale_;
      time image_;
      SQLLEN size_ind_;
    };

    // id_datetime.
    //
    template <typename T>
    struct query_param_impl<T, id_datetime>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            scale_ (r.scale != 0xFFFF ? r.scale : 7) // Default to datetime2/7.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            scale_ (v.scale != 0xFFFF ? v.scale : 7) // Default to datetime2/7.
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::datetime;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
        b->capacity = static_cast<SQLLEN> (scale_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_datetime>::set_image (image_, scale_, is_null, v);
        size_ind_ = 0;
      }

    private:
      unsigned short scale_;
      datetime image_;
      SQLLEN size_ind_;
    };

    // id_datetimeoffset.
    //
    template <typename T>
    struct query_param_impl<T, id_datetimeoffset>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (r.ptr ()),
            scale_ (r.scale != 0xFFFF ? r.scale : 7) // Default is 7.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            scale_ (v.scale != 0xFFFF ? v.scale : 7) // Default is 7.
      {
        init (v.val);
      }

      virtual bool
      init ()
      {
        init (*static_cast<const T*> (value_));
        return false;
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::datetimeoffset;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
        b->capacity = static_cast<SQLLEN> (scale_);
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_datetimeoffset>::set_image (
          image_, scale_, is_null, v);
        size_ind_ = static_cast<SQLLEN> (sizeof (image_));
      }

    private:
      unsigned short scale_;
      datetimeoffset image_;
      SQLLEN size_ind_;
    };

    // id_uniqueidentifier.
    //
    template <typename T>
    struct query_param_impl<T, id_uniqueidentifier>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::uniqueidentifier;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_uniqueidentifier>::set_image (image_, is_null, v);
        size_ind_ = 0;
      }

    private:
      uniqueidentifier image_;
      SQLLEN size_ind_;
    };

    // id_rowversion.
    //
    template <typename T>
    struct query_param_impl<T, id_rowversion>: query_param
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
      bind (bind_type* b)
      {
        b->type = bind::rowversion;
        b->buffer = &image_;
        b->size_ind = &size_ind_;
      }

    private:
      void
      init (typename decay_traits<T>::type v)
      {
        bool is_null (false); // Can't be NULL.
        value_traits<T, id_rowversion>::set_image (image_, is_null, v);
        size_ind_ = 8;
      }

    private:
      unsigned char image_[8];
      SQLLEN size_ind_;
    };
  }
}

// odb::mssql::query and odb::query specialization for SQL Server.
//
namespace odb
{
  namespace mssql
  {
    template <typename T>
    class query: public query_base,
                 public query_selector<T, id_mssql>::columns_type
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
      using mssql::query;
    }
  }

  // Derive odb::query from odb::mssql::query so that it can be
  // implicitly converted in mssql::database::query() calls.
  //
  template <typename T>
  class query<T, mssql::query_base>: public mssql::query<T>
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
        : mssql::query<T> (v)
    {
    }

    explicit
    query (const char* q)
        : mssql::query<T> (q)
    {
    }

    explicit
    query (const std::string& q)
        : mssql::query<T> (q)
    {
    }

    template <typename T2>
    explicit
    query (mssql::val_bind<T2> v)
        : mssql::query<T> (v)
    {
    }

    template <typename T2>
    explicit
    query (mssql::ref_bind<T2> r)
        : mssql::query<T> (r)
    {
    }

    query (const mssql::query_base& q)
        : mssql::query<T> (q)
    {
    }

    template <mssql::database_type_id ID>
    query (const mssql::query_column<bool, ID>& qc)
        : mssql::query<T> (qc)
    {
    }
  };
}

#include <odb/mssql/query.ixx>
#include <odb/mssql/query.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_QUERY_HXX
