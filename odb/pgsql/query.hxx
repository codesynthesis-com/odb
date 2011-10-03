// file      : odb/pgsql/query.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_QUERY_HXX
#define ODB_PGSQL_QUERY_HXX

#include <odb/pre.hxx>

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/query.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/traits.hxx>
#include <odb/pgsql/binding.hxx>
#include <odb/pgsql/pgsql-oid.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename T>
    class val_bind
    {
    public:
      explicit
      val_bind (const T& v): val (v) {}

      const T& val;
    };

    template <typename T>
    class ref_bind
    {
    public:
      explicit
      ref_bind (const T& r): ref (r) {}

      const T& ref;
    };

    struct LIBODB_PGSQL_EXPORT query_param: details::shared_base
    {
      typedef pgsql::bind bind_type;

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

      virtual unsigned int
      oid () const = 0;

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

    class LIBODB_PGSQL_EXPORT query
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
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
      {
      }

      // True or false literal.
      //
      explicit
      query (bool v)
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
      {
        clause_.push_back (clause_part (v));
      }

      explicit
      query (const char* native)
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
      {
        clause_.push_back (clause_part (clause_part::native, native));
      }

      explicit
      query (const std::string& native)
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
      {
        clause_.push_back (clause_part (clause_part::native, native));
      }

      query (const char* table, const char* column)
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
      {
        append (table, column);
      }

      template <typename T>
      explicit
      query (val_bind<T> v)
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
      {
        append<T, type_traits<T>::db_type_id> (v);
      }

      template <typename T>
      explicit
      query (ref_bind<T> r)
        : binding_ (0, 0), native_binding_ (0, 0, 0, 0)
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

      native_binding&
      parameters_binding () const;

      const unsigned int*
      parameter_types () const
      {
        return types_.empty () ? 0 : &types_[0];
      }

      std::size_t
      parameter_count () const
      {
        return parameters_.size ();
      }

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
      _val (const T& x)
      {
        return val_bind<T> (x);
      }

      template <typename T>
      static ref_bind<T>
      _ref (const T& x)
      {
        return ref_bind<T> (x);
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

      std::vector<char*> values_;
      std::vector<int> lengths_;
      std::vector<int> formats_;
      std::vector<unsigned int> types_;
      mutable native_binding native_binding_;
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

    LIBODB_PGSQL_EXPORT query
    operator&& (const query&, const query&);

    LIBODB_PGSQL_EXPORT query
    operator|| (const query&, const query&);

    LIBODB_PGSQL_EXPORT query
    operator! (const query&);

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
      query_column (const char* table, const char* column)
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

    // BOOLEAN
    //
    template <typename T>
    struct query_param_impl<T, id_boolean>: query_param
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
        b->type = bind::boolean;
        b->buffer = &image_;
      }

      virtual unsigned int
      oid () const
      {
        return bool_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_boolean>::set_image (image_, dummy, v);
      }

    private:
      bool image_;
    };

    // SMALLINT
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
      }

      virtual unsigned int
      oid () const
      {
        return int2_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_smallint>::set_image (image_, dummy, v);
      }

    private:
      short image_;
    };

    // INTEGER
    //
    template <typename T>
    struct query_param_impl<T, id_integer>: query_param
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
        b->type = bind::integer;
        b->buffer = &image_;
      }

      virtual unsigned int
      oid () const
      {
        return int4_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_integer>::set_image (image_, dummy, v);
      }

    private:
      int image_;
    };

    // BIGINT
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
      }

      virtual unsigned int
      oid () const
      {
        return int8_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_bigint>::set_image (image_, dummy, v);
      }

    private:
      long long image_;
    };

    // REAL
    //
    template <typename T>
    struct query_param_impl<T, id_real>: query_param
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
        b->type = bind::real;
        b->buffer = &image_;
      }

      virtual unsigned int
      oid () const
      {
        return float4_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_real>::set_image (image_, dummy, v);
      }

    private:
      float image_;
    };

    // DOUBLE
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
        b->type = bind::double_;
        b->buffer = &image_;
      }

      virtual unsigned int
      oid () const
      {
        return float8_oid;
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

    // NUMERIC
    //
    template <typename T>
    struct query_param_impl<T, id_numeric>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::numeric;
        b->buffer = buffer_.data ();
        b->capacity = buffer_.capacity ();
        b->size = &size_;
      }

      virtual unsigned int
      oid () const
      {
        return numeric_oid;
      }

    private:
      bool
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_numeric>::set_image (buffer_, size, dummy, v);
        size_ = size;
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      std::size_t size_;
    };

    // DATE
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
      }

      virtual unsigned int
      oid () const
      {
        return date_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_date>::set_image (image_, dummy, v);
      }

    private:
      int image_;
    };

    // TIME
    //
    template <typename T>
    struct query_param_impl<T, id_time>: query_param
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
        b->type = bind::time;
        b->buffer = &image_;
      }

      virtual unsigned int
      oid () const
      {
        return time_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_time>::set_image (image_, dummy, v);
      }

    private:
      long long image_;
    };

    // TIMESTAMP
    //
    template <typename T>
    struct query_param_impl<T, id_timestamp>: query_param
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
        b->type = bind::timestamp;
        b->buffer = &image_;
      }

      virtual unsigned int
      oid () const
      {
        return timestamp_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_timestamp>::set_image (image_, dummy, v);
      }

    private:
      long long image_;
    };

    // STRING
    //
    template <typename T>
    struct query_param_impl<T, id_string>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::text;
        b->buffer = buffer_.data ();
        b->capacity = buffer_.capacity ();
        b->size = &size_;
      }

      virtual unsigned int
      oid () const
      {
        return text_oid;
      }

    private:
      bool
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_string>::set_image (buffer_, size, dummy, v);
        size_ = size;
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      std::size_t size_;
    };

    // BYTEA
    //
    template <typename T>
    struct query_param_impl<T, id_bytea>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::bytea;
        b->buffer = buffer_.data ();
        b->capacity = buffer_.capacity ();
        b->size = &size_;
      }

      virtual unsigned int
      oid () const
      {
        return bytea_oid;
      }

    private:
      bool
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_bytea>::set_image (buffer_, size, dummy, v);
        size_ = size;
        return cap != buffer_.capacity ();
      }

    private:
      details::buffer buffer_;
      std::size_t size_;
    };

    // BIT
    //
    template <typename T>
    struct query_param_impl<T, id_bit>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::bit;
        b->buffer = buffer_.data ();
        b->capacity = buffer_.capacity ();
        b->size = &size_;
      }

    private:
      bool
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0), cap (buffer_.capacity ());

        // NOTE: Using a fixed size bit type in queries requires
        // alternative image buffer type support.
        //
        value_traits<T, id_bit>::set_image (buffer_, size, dummy, v);
        size_ = size;

        return cap != buffer_.capacity ();
      }

    private:
      details::ubuffer buffer_;
      std::size_t size_;
    };

    // VARBIT
    //
    template <typename T>
    struct query_param_impl<T, id_varbit>: query_param
    {
      query_param_impl (ref_bind<T> r) : query_param (&r.ref) {}
      query_param_impl (val_bind<T> v) : query_param (0) {init (v.val);}

      virtual bool
      init ()
      {
        return init (*static_cast<const T*> (value_));
      }

      virtual void
      bind (bind_type* b)
      {
        b->type = bind::varbit;
        b->buffer = buffer_.data ();
        b->capacity = buffer_.capacity ();
        b->size = &size_;
      }

      virtual unsigned int
      oid () const
      {
        return varbit_oid;
      }

    private:
      bool
      init (const T& v)
      {
        bool dummy;
        std::size_t size (0), cap (buffer_.capacity ());
        value_traits<T, id_varbit>::set_image (buffer_, size, dummy, v);
        size_ = size;
        return cap != buffer_.capacity ();
      }

    private:
      details::ubuffer buffer_;
      std::size_t size_;
    };

    // UUID
    //
    template <typename T>
    struct query_param_impl<T, id_uuid>: query_param
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
        b->type = bind::uuid;
        b->buffer = buffer_;
      }

      virtual unsigned int
      oid () const
      {
        return uuid_oid;
      }

    private:
      void
      init (const T& v)
      {
        bool dummy;
        value_traits<T, id_uuid>::set_image (buffer_, dummy, v);
      }

    private:
      unsigned char buffer_[16];
    };
  }
}

// odb::query specialization for PostgreSQL.
//
namespace odb
{
  template <typename T>
  class query<T, pgsql::query>: public query_selector<T>::type
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
    query (pgsql::val_bind<T2> v)
        : query_selector<T>::type (pgsql::query (v))
    {
    }

    template <typename T2>
    explicit
    query (pgsql::ref_bind<T2> r)
        : query_selector<T>::type (pgsql::query (r))
    {
    }

    query (const pgsql::query& q)
        : query_selector<T>::type (q)
    {
    }

    template <pgsql::database_type_id ID>
    query (const pgsql::query_column<bool, ID>& qc)
        : query_selector<T>::type (qc)
    {
    }
  };
}

#include <odb/pgsql/query.ixx>
#include <odb/pgsql/query.txx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_QUERY_HXX
