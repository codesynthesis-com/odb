// file      : odb/mssql/query.hxx
// author    : Contantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_QUERY_HXX
#define ODB_MSSQL_QUERY_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

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
    class val_bind
    {
    public:
      explicit
      val_bind (const T& v, unsigned short p = 0, unsigned short s = 0xFFFF)
          : val (v), prec (p), scale (s)
      {
      }

      const T& val;

      unsigned short prec;
      unsigned short scale;
    };

    template <typename T>
    class ref_bind
    {
    public:
      explicit
      ref_bind (const T& r, unsigned short p = 0, unsigned short s = 0xFFFF)
          : ref (r) , prec (p), scale (s)
      {
      }

      const T& ref;

      unsigned short prec;
      unsigned short scale;
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

    class LIBODB_MSSQL_EXPORT query
    {
    public:
      struct clause_part
      {
        enum kind_type
        {
          column,
          param,
          native,
          boolean
        };

        clause_part (kind_type k): kind (k) {}
        clause_part (kind_type k, const std::string& p): kind (k), part (p) {}
        clause_part (bool p): kind (boolean), bool_part (p) {}

        kind_type kind;
        std::string part;
        bool bool_part;
      };

      query ()
        : binding_ (0, 0)
      {
      }

      // True or false literal.
      //
      explicit
      query (bool v)
        : binding_ (0, 0)
      {
        clause_.push_back (clause_part (v));
      }

      explicit
      query (const char* native)
        : binding_ (0, 0)
      {
        clause_.push_back (clause_part (clause_part::native, native));
      }

      explicit
      query (const std::string& native)
        : binding_ (0, 0)
      {
        clause_.push_back (clause_part (clause_part::native, native));
      }

      query (const char* table, const char* column)
        : binding_ (0, 0)
      {
        append (table, column);
      }

      template <typename T>
      explicit
      query (val_bind<T> v)
        : binding_ (0, 0)
      {
        append<T, type_traits<T>::db_type_id> (v);
      }

      template <typename T>
      explicit
      query (ref_bind<T> r)
        : binding_ (0, 0)
      {
        append<T, type_traits<T>::db_type_id> (r);
      }

      template <database_type_id ID>
      query (const query_column<bool, ID>&);

      query (const query&);

      query&
      operator= (const query&);

    public:
      std::string
      clause () const;

      const char*
      clause_prefix () const;

      binding&
      parameters_binding () const;

    public:
      bool
      empty () const
      {
        return clause_.empty ();
      }

      static const query true_expr;

      bool
      const_true () const
      {
        return clause_.size () == 1 &&
          clause_.front ().kind == clause_part::boolean &&
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

      template <typename T>
      static ref_bind<T>
      _ref (const T& x, unsigned short prec = 0, unsigned short scale = 0xFFFF)
      {
        return ref_bind<T> (x, prec, scale);
      }

    public:
      query&
      operator+= (const query&);

      query&
      operator+= (const std::string& q)
      {
        append (q);
        return *this;
      }

      template <typename T>
      query&
      operator+= (val_bind<T> v)
      {
        append<T, type_traits<T>::db_type_id> (v);
        return *this;
      }

      template <typename T>
      query&
      operator+= (ref_bind<T> r)
      {
        append<T, type_traits<T>::db_type_id> (r);
        return *this;
      }

    public:
      template <typename T, database_type_id ID>
      void
      append (val_bind<T>);

      template <typename T, database_type_id ID>
      void
      append (ref_bind<T>);

      void
      append (const std::string& native);

      void
      append (const char* table, const char* column);

    private:
      void
      add (details::shared_ptr<query_param>);

    private:
      typedef std::vector<clause_part> clause_type;
      typedef std::vector<details::shared_ptr<query_param> > parameters_type;

      clause_type clause_;
      parameters_type parameters_;
      mutable std::vector<bind> bind_;
      mutable binding binding_;
    };

    inline query
    operator+ (const query& x, const query& y)
    {
      query r (x);
      r += y;
      return r;
    }

    template <typename T>
    inline query
    operator+ (const query& q, val_bind<T> b)
    {
      query r (q);
      r += b;
      return r;
    }

    template <typename T>
    inline query
    operator+ (const query& q, ref_bind<T> b)
    {
      query r (q);
      r += b;
      return r;
    }

    template <typename T>
    inline query
    operator+ (val_bind<T> b, const query& q)
    {
      query r;
      r += b;
      r += q;
      return r;
    }

    template <typename T>
    inline query
    operator+ (ref_bind<T> b, const query& q)
    {
      query r;
      r += b;
      r += q;
      return r;
    }

    inline query
    operator+ (const query& q, const std::string& s)
    {
      query r (q);
      r += s;
      return r;
    }

    inline query
    operator+ (const std::string& s, const query& q)
    {
      query r (s);
      r += q;
      return r;
    }

    template <typename T>
    inline query
    operator+ (const std::string& s, val_bind<T> b)
    {
      query r (s);
      r += b;
      return r;
    }

    template <typename T>
    inline query
    operator+ (const std::string& s, ref_bind<T> b)
    {
      query r (s);
      r += b;
      return r;
    }

    template <typename T>
    inline query
    operator+ (val_bind<T> b, const std::string& s)
    {
      query r;
      r += b;
      r += s;
      return r;
    }

    template <typename T>
    inline query
    operator+ (ref_bind<T> b, const std::string& s)
    {
      query r;
      r += b;
      r += s;
      return r;
    }

    LIBODB_MSSQL_EXPORT query
    operator&& (const query& x, const query& y);

    LIBODB_MSSQL_EXPORT query
    operator|| (const query& x, const query& y);

    LIBODB_MSSQL_EXPORT query
    operator! (const query& x);

    // query_column
    //

    template <typename T, typename T2>
    class copy_bind: public val_bind<T>
    {
    public:
      explicit
      copy_bind (const T2& v): val_bind<T> (val), val (v) {}

      const T val;
    };

    template <typename T>
    const T&
    type_instance ();

    template <typename T, database_type_id ID>
    struct query_column
    {
      // Note that we keep shalow copies of the table and column names.
      //
      query_column (const char* table,
                    const char* column,
                    unsigned short prec = 0,
                    unsigned short scale = 0xFFFF)
          : table_ (table), column_ (column), prec_ (prec), scale_ (scale)
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

      // is_null, is_not_null
      //
    public:
      query
      is_null () const
      {
        query q (table_, column_);
        q += "IS NULL";
        return q;
      }

      query
      is_not_null () const
      {
        query q (table_, column_);
        q += "IS NOT NULL";
        return q;
      }

      // in
      //
    public:
      query
      in (const T&, const T&) const;

      query
      in (const T&, const T&, const T&) const;

      query
      in (const T&, const T&, const T&, const T&) const;

      query
      in (const T&, const T&, const T&, const T&, const T&) const;

      template <typename I>
      query
      in_range (I begin, I end) const;

      // =
      //
    public:
      query
      equal (const T& v) const
      {
        return equal (val_bind<T> (v));
      }

      query
      equal (val_bind<T> v) const
      {
        v.prec = prec_;
        v.scale = scale_;

        query q (table_, column_);
        q += "=";
        q.append<T, ID> (v);
        return q;
      }

      template <typename T2>
      query
      equal (val_bind<T2> v) const
      {
        copy_bind<T, T2> c (v.val);
        return equal (c);
      }

      query
      equal (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

        query q (table_, column_);
        q += "=";
        q.append<T, ID> (r);
        return q;
      }

      friend query
      operator== (const query_column& c, const T& v)
      {
        return c.equal (v);
      }

      friend query
      operator== (const T& v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query
      operator== (const query_column& c, val_bind<T> v)
      {
        return c.equal (v);
      }

      friend query
      operator== (val_bind<T> v, const query_column& c)
      {
        return c.equal (v);
      }

      template <typename T2>
      friend query
      operator== (const query_column& c, val_bind<T2> v)
      {
        return c.equal (v);
      }

      template <typename T2>
      friend query
      operator== (val_bind<T2> v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query
      operator== (const query_column& c, ref_bind<T> r)
      {
        return c.equal (r);
      }

      friend query
      operator== (ref_bind<T> r, const query_column& c)
      {
        return c.equal (r);
      }

      // !=
      //
    public:
      query
      unequal (const T& v) const
      {
        return unequal (val_bind<T> (v));
      }

      query
      unequal (val_bind<T> v) const
      {
        v.prec = prec_;
        v.scale = scale_;

        query q (table_, column_);
        q += "!=";
        q.append<T, ID> (v);
        return q;
      }

      template <typename T2>
      query
      unequal (val_bind<T2> v) const
      {
        copy_bind<T, T2> c (v.val);
        return unequal (c);
      }

      query
      unequal (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

        query q (table_, column_);
        q += "!=";
        q.append<T, ID> (r);
        return q;
      }

      friend query
      operator!= (const query_column& c, const T& v)
      {
        return c.unequal (v);
      }

      friend query
      operator!= (const T& v, const query_column& c)
      {
        return c.unequal (v);
      }

      friend query
      operator!= (const query_column& c, val_bind<T> v)
      {
        return c.unequal (v);
      }

      friend query
      operator!= (val_bind<T> v, const query_column& c)
      {
        return c.unequal (v);
      }

      template <typename T2>
      friend query
      operator!= (const query_column& c, val_bind<T2> v)
      {
        return c.unequal (v);
      }

      template <typename T2>
      friend query
      operator!= (val_bind<T2> v, const query_column& c)
      {
        return c.unequal (v);
      }

      friend query
      operator!= (const query_column& c, ref_bind<T> r)
      {
        return c.unequal (r);
      }

      friend query
      operator!= (ref_bind<T> r, const query_column& c)
      {
        return c.unequal (r);
      }

      // <
      //
    public:
      query
      less (const T& v) const
      {
        return less (val_bind<T> (v));
      }

      query
      less (val_bind<T> v) const
      {
        v.prec = prec_;
        v.scale = scale_;

        query q (table_, column_);
        q += "<";
        q.append<T, ID> (v);
        return q;
      }

      template <typename T2>
      query
      less (val_bind<T2> v) const
      {
        copy_bind<T, T2> c (v.val);
        return less (c);
      }

      query
      less (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

        query q (table_, column_);
        q += "<";
        q.append<T, ID> (r);
        return q;
      }

      friend query
      operator< (const query_column& c, const T& v)
      {
        return c.less (v);
      }

      friend query
      operator< (const T& v, const query_column& c)
      {
        return c.greater (v);
      }

      friend query
      operator< (const query_column& c, val_bind<T> v)
      {
        return c.less (v);
      }

      friend query
      operator< (val_bind<T> v, const query_column& c)
      {
        return c.greater (v);
      }

      template <typename T2>
      friend query
      operator< (const query_column& c, val_bind<T2> v)
      {
        return c.less (v);
      }

      template <typename T2>
      friend query
      operator< (val_bind<T2> v, const query_column& c)
      {
        return c.greater (v);
      }

      friend query
      operator< (const query_column& c, ref_bind<T> r)
      {
        return c.less (r);
      }

      friend query
      operator< (ref_bind<T> r, const query_column& c)
      {
        return c.greater (r);
      }

      // >
      //
    public:
      query
      greater (const T& v) const
      {
        return greater (val_bind<T> (v));
      }

      query
      greater (val_bind<T> v) const
      {
        v.prec = prec_;
        v.scale = scale_;

        query q (table_, column_);
        q += ">";
        q.append<T, ID> (v);
        return q;
      }

      template <typename T2>
      query
      greater (val_bind<T2> v) const
      {
        copy_bind<T, T2> c (v.val);
        return greater (c);
      }

      query
      greater (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

        query q (table_, column_);
        q += ">";
        q.append<T, ID> (r);
        return q;
      }

      friend query
      operator> (const query_column& c, const T& v)
      {
        return c.greater (v);
      }

      friend query
      operator> (const T& v, const query_column& c)
      {
        return c.less (v);
      }

      friend query
      operator> (const query_column& c, val_bind<T> v)
      {
        return c.greater (v);
      }

      friend query
      operator> (val_bind<T> v, const query_column& c)
      {
        return c.less (v);
      }

      template <typename T2>
      friend query
      operator> (const query_column& c, val_bind<T2> v)
      {
        return c.greater (v);
      }

      template <typename T2>
      friend query
      operator> (val_bind<T2> v, const query_column& c)
      {
        return c.less (v);
      }

      friend query
      operator> (const query_column& c, ref_bind<T> r)
      {
        return c.greater (r);
      }

      friend query
      operator> (ref_bind<T> r, const query_column& c)
      {
        return c.less (r);
      }

      // <=
      //
    public:
      query
      less_equal (const T& v) const
      {
        return less_equal (val_bind<T> (v));
      }

      query
      less_equal (val_bind<T> v) const
      {
        v.prec = prec_;
        v.scale = scale_;

        query q (table_, column_);
        q += "<=";
        q.append<T, ID> (v);
        return q;
      }

      template <typename T2>
      query
      less_equal (val_bind<T2> v) const
      {
        copy_bind<T, T2> c (v.val);
        return less_equal (c);
      }

      query
      less_equal (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

        query q (table_, column_);
        q += "<=";
        q.append<T, ID> (r);
        return q;
      }

      friend query
      operator<= (const query_column& c, const T& v)
      {
        return c.less_equal (v);
      }

      friend query
      operator<= (const T& v, const query_column& c)
      {
        return c.greater_equal (v);
      }

      friend query
      operator<= (const query_column& c, val_bind<T> v)
      {
        return c.less_equal (v);
      }

      friend query
      operator<= (val_bind<T> v, const query_column& c)
      {
        return c.greater_equal (v);
      }

      template <typename T2>
      friend query
      operator<= (const query_column& c, val_bind<T2> v)
      {
        return c.less_equal (v);
      }

      template <typename T2>
      friend query
      operator<= (val_bind<T2> v, const query_column& c)
      {
        return c.greater_equal (v);
      }

      friend query
      operator<= (const query_column& c, ref_bind<T> r)
      {
        return c.less_equal (r);
      }

      friend query
      operator<= (ref_bind<T> r, const query_column& c)
      {
        return c.greater_equal (r);
      }

      // >=
      //
    public:
      query
      greater_equal (const T& v) const
      {
        return greater_equal (val_bind<T> (v));
      }

      query
      greater_equal (val_bind<T> v) const
      {
        v.prec = prec_;
        v.scale = scale_;

        query q (table_, column_);
        q += ">=";
        q.append<T, ID> (v);
        return q;
      }

      template <typename T2>
      query
      greater_equal (val_bind<T2> v) const
      {
        copy_bind<T, T2> c (v.val);
        return greater_equal (c);
      }

      query
      greater_equal (ref_bind<T> r) const
      {
        r.prec = prec_;
        r.scale = scale_;

        query q (table_, column_);
        q += ">=";
        q.append<T, ID> (r);
        return q;
      }

      friend query
      operator>= (const query_column& c, const T& v)
      {
        return c.greater_equal (v);
      }

      friend query
      operator>= (const T& v, const query_column& c)
      {
        return c.less_equal (v);
      }

      friend query
      operator>= (const query_column& c, val_bind<T> v)
      {
        return c.greater_equal (v);
      }

      friend query
      operator>= (val_bind<T> v, const query_column& c)
      {
        return c.less_equal (v);
      }

      template <typename T2>
      friend query
      operator>= (const query_column& c, val_bind<T2> v)
      {
        return c.greater_equal (v);
      }

      template <typename T2>
      friend query
      operator>= (val_bind<T2> v, const query_column& c)
      {
        return c.less_equal (v);
      }

      friend query
      operator>= (const query_column& c, ref_bind<T> r)
      {
        return c.greater_equal (r);
      }

      friend query
      operator>= (ref_bind<T> r, const query_column& c)
      {
        return c.less_equal (r);
      }

      // Column comparison.
      //
    public:
      template <typename T2, database_type_id ID2>
      query
      operator== (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (type_instance<T> () == type_instance<T2> ()));

        query q (table_, column_);
        q += "=";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query
      operator!= (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (type_instance<T> () != type_instance<T2> ()));

        query q (table_, column_);
        q += "!=";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query
      operator< (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (type_instance<T> () < type_instance<T2> ()));

        query q (table_, column_);
        q += "<";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query
      operator> (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (type_instance<T> () > type_instance<T2> ()));

        query q (table_, column_);
        q += ">";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query
      operator<= (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (type_instance<T> () <= type_instance<T2> ()));

        query q (table_, column_);
        q += "<=";
        q.append (c.table (), c.column ());
        return q;
      }

      template <typename T2, database_type_id ID2>
      query
      operator>= (const query_column<T2, ID2>& c) const
      {
        // We can compare columns only if we can compare their C++ types.
        //
        (void) (sizeof (type_instance<T> () >= type_instance<T2> ()));

        query q (table_, column_);
        q += ">=";
        q.append (c.table (), c.column ());
        return q;
      }

    private:
      const char* table_;
      const char* column_;

      unsigned short prec_;
      unsigned short scale_;
    };

    // Provide operator+() for using columns to construct native
    // query fragments (e.g., ORDER BY).
    //
    template <typename T, database_type_id ID>
    inline query
    operator+ (const query_column<T, ID>& c, const std::string& s)
    {
      query q (c.table (), c.column ());
      q += s;
      return q;
    }

    template <typename T, database_type_id ID>
    inline query
    operator+ (const std::string& s, const query_column<T, ID>& c)
    {
      query q (s);
      q.append (c.table (), c.column ());
      return q;
    }

    template <typename T, database_type_id ID>
    inline query
    operator+ (const query_column<T, ID>& c, const query& q)
    {
      query r (c.table (), c.column ());
      r += q;
      return r;
    }

    template <typename T, database_type_id ID>
    inline query
    operator+ (const query& q, const query_column<T, ID>& c)
    {
      query r (q);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_bit>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_tinyint>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_smallint>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_int>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_bigint>::set_image (image_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_decimal>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_smallmoney>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_money>::set_image (image_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_float4>::set_image (image_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_float8>::set_image (image_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_string>::set_image (
          buf_.data (), buf_.capacity (), size, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_nstring>::set_image (
          reinterpret_cast<ucs2_char*> (buf_.data ()),
          buf_.capacity (),
          size,
          dummy,
          v);
        size_ind_ = static_cast<SQLLEN> (size);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_binary>::set_image (
          buf_.data (), buf_.capacity (), size, dummy, v);
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
          : query_param (&r.ref), prec_ (r.prec)
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_long_string>::set_image (
          long_query_param_impl::callback_.callback.param,
          long_query_param_impl::callback_.context.param,
          dummy,
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
          : query_param (&r.ref), prec_ (r.prec)
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_long_nstring>::set_image (
          long_query_param_impl::callback_.callback.param,
          long_query_param_impl::callback_.context.param,
          dummy,
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
          : query_param (&r.ref), prec_ (r.prec)
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_long_binary>::set_image (
          long_query_param_impl::callback_.callback.param,
          long_query_param_impl::callback_.context.param,
          dummy,
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_date>::set_image (image_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_time>::set_image (image_, scale_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_datetime>::set_image (image_, scale_, dummy, v);
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
          : query_param (&r.ref),
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_datetimeoffset>::set_image (
          image_, scale_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_uniqueidentifier>::set_image (image_, dummy, v);
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
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
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
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_rowversion>::set_image (image_, dummy, v);
        size_ind_ = 8;
      }

    private:
      unsigned char image_[8];
      SQLLEN size_ind_;
    };
  }
}

// odb::query specialization for SQL Server.
//
namespace odb
{
  template <typename T>
  class query<T, mssql::query>: public query_selector<T>::type
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
        : query_selector<T>::type (v)
    {
    }

    explicit
    query (const char* q)
        : query_selector<T>::type (q)
    {
    }

    explicit
    query (const std::string& q)
        : query_selector<T>::type (q)
    {
    }

    template <typename T2>
    explicit
    query (mssql::val_bind<T2> v)
        : query_selector<T>::type (mssql::query (v))
    {
    }

    template <typename T2>
    explicit
    query (mssql::ref_bind<T2> r)
        : query_selector<T>::type (mssql::query (r))
    {
    }

    query (const mssql::query& q)
        : query_selector<T>::type (q)
    {
    }

    template <mssql::database_type_id ID>
    query (const mssql::query_column<bool, ID>& qc)
        : query_selector<T>::type (qc)
    {
    }
  };
}

#include <odb/mssql/query.ixx>
#include <odb/mssql/query.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_QUERY_HXX
