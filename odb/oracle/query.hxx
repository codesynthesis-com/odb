// file      : odb/oracle/query.hxx
// author    : Contantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_QUERY_HXX
#define ODB_ORACLE_QUERY_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/query.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/traits.hxx>
#include <odb/oracle/binding.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/shared-ptr.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    // For both precision and scale 0 is a valid value. Furthermore,
    // scale can be negative. To indicate that these values are not
    // specified, we will use 0xFFF which is out of range for both
    // precision (0 to 4000) and scale (-84 to 127). Note also that
    // string size (stored in precision) is always in bytes. If a
    // national string size is specified as a number of characters
    // and not bytes, then this will be a conservative estimate (4
    // bytes per character).
    //
    template <typename T>
    class val_bind
    {
    public:
      explicit
      val_bind (const T& v, unsigned short p = 0xFFF, short s = 0xFFF)
          : val (v), prec (p), scale (s)
      {
      }

      const T& val;

      unsigned short prec;
      short scale;
    };

    template <typename T>
    class ref_bind
    {
    public:
      explicit
      ref_bind (const T& r, unsigned short p = 0xFFF, short s = 0xFFF)
          : ref (r), prec (p), scale (s)
      {
      }

      const T& ref;

      unsigned short prec;
      short scale;
    };

    struct LIBODB_ORACLE_EXPORT query_param: details::shared_base
    {
      typedef oracle::bind bind_type;

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

    class LIBODB_ORACLE_EXPORT query
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
      _val (const T& x, unsigned short prec = 0xFFF, short scale = 0xFFF)
      {
        return val_bind<T> (x, prec, scale);
      }

      template <typename T>
      static ref_bind<T>
      _ref (const T& x, unsigned short prec = 0xFFF, short scale = 0xFFF)
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

    LIBODB_ORACLE_EXPORT query
    operator&& (const query& x, const query& y);

    LIBODB_ORACLE_EXPORT query
    operator|| (const query& x, const query& y);

    LIBODB_ORACLE_EXPORT query
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
                    unsigned short prec = 0xFFF,
                    short scale = 0xFFF)
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

      short
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
      short scale_;
    };

    //
    // Oracle does not support comparison operations between LOB columns.
    // query_column therefore only supports the IS NULL and IS NOT NULL
    // predicates for these types.
    //

    struct LIBODB_ORACLE_EXPORT lob_query_column
    {
      // Note that we keep shalow copies of the table and column names.
      //
      lob_query_column (const char* table, const char* column)
          : table_ (table), column_ (column)
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

    private:
      const char* table_;
      const char* column_;
    };

    template <typename T>
    struct query_column<T, id_blob>: lob_query_column
    {
      query_column (const char* table, const char* column)
          : lob_query_column (table, column)
      {
      }
    };

    template <typename T>
    struct query_column<T, id_clob>
    {
      query_column (const char* table, const char* column)
          : lob_query_column (table, column)
      {
      }
    };

    template <typename T>
    struct query_column<T, id_nclob>
    {
      query_column (const char* table, const char* column)
          : lob_query_column (table, column)
      {
      }
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

    // id_int32.
    //
    template <typename T>
    struct query_param_impl<T, id_int32>: query_param
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
        b->type = image_traits<T, id_int32>::buffer_type;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_int32>::set_image (image_, dummy, v);
      }

    private:
      typename image_traits<T, id_int32>::image_type image_;
    };

    // id_int64.
    //
    template <typename T>
    struct query_param_impl<T, id_int64>: query_param
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
        b->type = image_traits<T, id_int64>::buffer_type;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_int64>::set_image (image_, dummy, v);
      }

    private:
      typename image_traits<T, id_int64>::image_type image_;
    };

    // id_big_int.
    //
    template <typename T>
    struct query_param_impl<T, id_big_int>: query_param
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
        b->type = bind_type::number;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = &size_;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_big_int>::set_image (image_, size, dummy, v);
        size_ = static_cast<ub2> (size);
      }

    private:
      char image_[21];
      ub2 size_;
    };

    // id_float.
    //
    template <typename T>
    struct query_param_impl<T, id_float>: query_param
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
        b->type = bind_type::binary_float;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_float>::set_image (image_, dummy, v);
      }

    private:
      float image_;
    };

    // id_double.
    //
    template <typename T>
    struct query_param_impl<T, id_double>: query_param
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
        b->type = bind_type::binary_double;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_double>::set_image (image_, dummy, v);
      }

    private:
      double image_;
    };

    // id_big_float.
    //
    template <typename T>
    struct query_param_impl<T, id_big_float>: query_param
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
        b->type = bind_type::number;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = &size_;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_big_float>::set_image (image_, size, dummy, v);
        size_ = static_cast<ub2> (size);
      }

    private:
      char image_[21];
      ub2 size_;
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
        b->type = bind_type::date;
        b->buffer = &image_;
        b->capacity = sizeof (image_);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_date>::set_image (image_, dummy, v);
      }

    private:
      char image_[7];
    };

    // id_timestamp
    //
    template <typename T>
    struct query_param_impl<T, id_timestamp>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (&r.ref),
            image_ (descriptor_cache) // Cache, don't free.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            image_ (0) // Don't cache, don't free.

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
        b->type = bind_type::timestamp;
        b->buffer = &image_;
        b->capacity = sizeof (OCIDateTime*);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_timestamp>::set_image (image_, dummy, v);
      }

    private:
      datetime image_;
    };

    // id_interval_ym
    //
    template <typename T>
    struct query_param_impl<T, id_interval_ym>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (&r.ref),
            image_ (descriptor_cache) // Cache, don't free.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            image_ (0) // Don't cache, don't free.

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
        b->type = bind_type::interval_ym;
        b->buffer = &image_;
        b->capacity = sizeof (OCIInterval*);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_interval_ym>::set_image (image_, dummy, v);
      }

    private:
      interval_ym image_;
    };

    // id_interval_ds
    //
    template <typename T>
    struct query_param_impl<T, id_interval_ds>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (&r.ref),
            image_ (descriptor_cache) // Cache, don't free.
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            image_ (0) // Don't cache, don't free.

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
        b->type = bind_type::interval_ds;
        b->buffer = &image_;
        b->capacity = sizeof (OCIInterval*);
        b->size = 0;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_interval_ds>::set_image (image_, dummy, v);
      }

    private:
      interval_ds image_;
    };

    // id_string.
    //
    template <typename T>
    struct query_param_impl<T, id_string>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (&r.ref),
            // Default to max (4000).
            buf_ (r.prec != 0xFFF ? r.prec : 4000)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            // Default to max (4000).
            buf_ (v.prec != 0xFFF ? v.prec : 4000)

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
        b->type = bind_type::string;
        b->buffer = buf_.data ();
        b->capacity = static_cast<ub4> (buf_.capacity ());
        b->size = &size_;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_string>::set_image (
          buf_.data (), buf_.capacity (), size, dummy, v);
        size_ = static_cast<ub2> (size);
      }

    private:
      details::buffer buf_;
      ub2 size_;
    };

    // id_nstring
    //
    template <typename T>
    struct query_param_impl<T, id_nstring>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (&r.ref),
            // Default to max (4000).
            buf_ (r.prec != 0xFFF ? r.prec : 4000)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            // Default to max (4000).
            buf_ (v.prec != 0xFFF ? v.prec : 4000)

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
        b->type = bind_type::nstring;
        b->buffer = buf_.data ();
        b->capacity = static_cast<ub4> (buf_.capacity ());
        b->size = &size_;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_nstring>::set_image (
          buf_.data (), buf_.capacity (), size, dummy, v);
        size_ = static_cast<ub2> (size);
      }

    private:
      details::buffer buf_;
      ub2 size_;
    };

    // id_raw
    //
    template <typename T>
    struct query_param_impl<T, id_raw>: query_param
    {
      query_param_impl (ref_bind<T> r)
          : query_param (&r.ref),
            // Default to max (2000).
            buf_ (r.prec != 0xFFF ? r.prec : 2000)
      {
      }

      query_param_impl (val_bind<T> v)
          : query_param (0),
            // Default to max (2000).
            buf_ (v.prec != 0xFFF ? v.prec : 2000)

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
        b->type = bind_type::raw;
        b->buffer = buf_.data ();
        b->capacity = static_cast<ub4> (buf_.capacity ());
        b->size = &size_;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0);
        value_traits<T, id_raw>::set_image (
          buf_.data (), buf_.capacity (), size, dummy, v);
        size_ = static_cast<ub2> (size);
      }

    private:
      details::buffer buf_;
      ub2 size_;
    };
  }
}

// odb::query specialization for Oracle.
//
namespace odb
{
  template <typename T>
  class query<T, oracle::query>: public query_selector<T>::type
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
    query (oracle::val_bind<T2> v)
        : query_selector<T>::type (oracle::query (v))
    {
    }

    template <typename T2>
    explicit
    query (oracle::ref_bind<T2> r)
        : query_selector<T>::type (oracle::query (r))
    {
    }

    query (const oracle::query& q)
        : query_selector<T>::type (q)
    {
    }

    template <oracle::database_type_id ID>
    query (const oracle::query_column<bool, ID>& qc)
        : query_selector<T>::type (qc)
    {
    }
  };
}

#include <odb/oracle/query.ixx>
#include <odb/oracle/query.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_QUERY_HXX
