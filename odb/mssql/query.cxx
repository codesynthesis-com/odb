// file      : odb/mssql/query.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset, std::memcpy

#include <odb/mssql/query.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    // query_param
    //
    query_param::
    ~query_param ()
    {
    }

    // query_base
    //
    query_base::
    query_base (const query_base& q)
        : clause_ (q.clause_),
          parameters_ (q.parameters_),
          bind_ (q.bind_),
          binding_ (0, 0)
    {
      // Here and below we want to maintain up to date binding info so
      // that the call to parameters_binding() below is an immutable
      // operation, provided the query does not have any by-reference
      // parameters. This way a by-value-only query can be shared
      // between multiple threads without the need for synchronization.
      //
      if (size_t n = bind_.size ())
      {
        binding_.bind = &bind_[0];
        binding_.count = n;
        binding_.version++;
      }
    }

    query_base& query_base::
    operator= (const query_base& q)
    {
      if (this != &q)
      {
        clause_ = q.clause_;
        parameters_ = q.parameters_;
        bind_ = q.bind_;

        size_t n (bind_.size ());
        binding_.bind = n != 0 ? &bind_[0] : 0;
        binding_.count = n;
        binding_.version++;
      }

      return *this;
    }

    query_base& query_base::
    operator+= (const query_base& q)
    {
      clause_.insert (clause_.end (), q.clause_.begin (), q.clause_.end ());

      size_t n (bind_.size ());

      parameters_.insert (
        parameters_.end (), q.parameters_.begin (), q.parameters_.end ());

      bind_.insert (
        bind_.end (), q.bind_.begin (), q.bind_.end ());

      if (n != bind_.size ())
      {
        binding_.bind = &bind_[0];
        binding_.count = bind_.size ();
        binding_.version++;
      }

      return *this;
    }

    void query_base::
    append (const string& q)
    {
      if (!clause_.empty () &&
          clause_.back ().kind == clause_part::kind_native)
      {
        string& s (clause_.back ().part);

        char first (!q.empty () ? q[0] : ' ');
        char last (!s.empty () ? s[s.size () - 1] : ' ');

        // We don't want extra spaces after '(' as well as before ','
        // and ')'.
        //
        if (last != ' ' && last != '\n' && last != '(' &&
            first != ' ' && first != '\n' && first != ',' && first != ')')
          s += ' ';

        s += q;
      }
      else
        clause_.push_back (clause_part (clause_part::kind_native, q));
    }

    void query_base::
    append (const char* table, const char* column)
    {
      string s (table);
      s += '.';
      s += column;

      clause_.push_back (clause_part (clause_part::kind_column, s));
    }

    void query_base::
    append (details::shared_ptr<query_param> p, const char* conv)
    {
      clause_.push_back (clause_part (clause_part::kind_param));

      if (conv != 0)
        clause_.back ().part = conv;

      parameters_.push_back (p);
      bind_.push_back (bind ());
      binding_.bind = &bind_[0];
      binding_.count = bind_.size ();
      binding_.version++;

      bind* b (&bind_.back ());
      memset (b, 0, sizeof (bind));
      p->bind (b);
    }

    void query_base::
    init_parameters () const
    {
      bool inc_ver (false);

      for (size_t i (0); i < parameters_.size (); ++i)
      {
        query_param& p (*parameters_[i]);

        if (p.reference ())
        {
          if (p.init ())
          {
            p.bind (&bind_[i]);
            inc_ver = true;
          }
        }
      }

      if (inc_ver)
        binding_.version++;
    }

    static bool
    check_prefix (const string& s)
    {
      string::size_type n;

      // It is easier to compare to upper and lower-case versions
      // rather than getting involved with the portable case-
      // insensitive string comparison mess.
      //
      if (s.compare (0, (n = 5), "WHERE") == 0 ||
          s.compare (0, (n = 5), "where") == 0 ||
          s.compare (0, (n = 6), "SELECT") == 0 ||
          s.compare (0, (n = 6), "select") == 0 ||
          s.compare (0, (n = 8), "ORDER BY") == 0 ||
          s.compare (0, (n = 8), "order by") == 0 ||
          s.compare (0, (n = 8), "GROUP BY") == 0 ||
          s.compare (0, (n = 8), "group by") == 0 ||
          s.compare (0, (n = 6), "HAVING") == 0 ||
          s.compare (0, (n = 6), "having") == 0 ||
          s.compare (0, (n = 4), "EXEC") == 0 ||
          s.compare (0, (n = 4), "exec") == 0 ||
          s.compare (0, (n = 7), "EXECUTE") == 0 ||
          s.compare (0, (n = 7), "execute") == 0)
      {
        // It either has to be an exact match, or there should be
        // a whitespace following the keyword.
        //
        if (s.size () == n || s[n] == ' ' || s[n] == '\n' || s[n] =='\t')
          return true;
      }

      return false;
    }

    void query_base::
    optimize ()
    {
      // Remove a single TRUE literal or one that is followe by one of
      // the other clauses. This avoids useless WHERE clauses like
      //
      // WHERE TRUE GROUP BY foo
      //
      clause_type::iterator i (clause_.begin ()), e (clause_.end ());

      if (i != e && i->kind == clause_part::kind_bool && i->bool_part)
      {
        clause_type::iterator j (i + 1);

        if (j == e ||
            (j->kind == clause_part::kind_native && check_prefix (j->part)))
          clause_.erase (i);
      }
    }

    const char* query_base::
    clause_prefix () const
    {
      if (!clause_.empty ())
      {
        const clause_part& p (clause_.front ());

        if (p.kind == clause_part::kind_native && check_prefix (p.part))
          return "";

        return "WHERE ";
      }

      return "";
    }

    string query_base::
    clause () const
    {
      string r;

      for (clause_type::const_iterator i (clause_.begin ()),
             end (clause_.end ());
           i != end;
           ++i)
      {
        char last (!r.empty () ? r[r.size () - 1] : ' ');

        switch (i->kind)
        {
        case clause_part::kind_column:
          {
            if (last != ' ' && last != '\n' && last != '(')
              r += ' ';

            r += i->part;
            break;
          }
        case clause_part::kind_param:
          {
            if (last != ' ' && last != '\n' && last != '(')
              r += ' ';

            // Add the conversion expression, if any.
            //
            string::size_type p (0);
            if (!i->part.empty ())
            {
              p = i->part.find ("(?)");
              r.append (i->part, 0, p);
            }

            r += '?';

            if (!i->part.empty ())
              r.append (i->part, p + 3, string::npos);

            break;
          }
        case clause_part::kind_native:
          {
            // We don't want extra spaces after '(' as well as before ','
            // and ')'.
            //
            const string& p (i->part);
            char first (!p.empty () ? p[0] : ' ');

            if (last != ' ' && last != '\n' && last != '(' &&
                first != ' ' && first != '\n' && first != ',' && first != ')')
              r += ' ';

            r += p;
            break;
          }
        case clause_part::kind_bool:
          {
            if (last != ' ' && last != '\n' && last != '(')
              r += ' ';

            // SQL Server does not have "true" TRUE and FALSE boolean
            // constants (there seem to be TRUE and FALSE literals but
            // they are interpreted as integers). Boolean values seem
            // to only be created as the result of boolean expressions.
            //
            r += i->bool_part ? "1 = 1" : "1 = 0";
            break;
          }
        }
      }

      return clause_prefix () + r;
    }

    query_base
    operator&& (const query_base& x, const query_base& y)
    {
      // Optimize cases where one or both sides are constant truth.
      //
      bool xt (x.const_true ()), yt (y.const_true ());

      if (xt && yt)
        return x;

      if (xt)
        return y;

      if (yt)
        return x;

      query_base r ("(");
      r += x;
      r += ") AND (";
      r += y;
      r += ")";
      return r;
    }

    query_base
    operator|| (const query_base& x, const query_base& y)
    {
      query_base r ("(");
      r += x;
      r += ") OR (";
      r += y;
      r += ")";
      return r;
    }

    query_base
    operator! (const query_base& x)
    {
      query_base r ("NOT (");
      r += x;
      r += ")";
      return r;
    }

    //
    // long_query_param_impl
    //

    void long_query_param_impl::
    copy ()
    {
      size_ = 0;

      // The below code is the same as in statement.cxx.
      //
      const void* buf (&callback_);
      char tmp_buf[1024];

      size_t position (0);
      for (;;)
      {
        size_t n;
        chunk_type chunk;

        callback_.callback.param (
          callback_.context.param,
          &position,
          &buf,
          &n,
          &chunk,
          tmp_buf,
          sizeof (tmp_buf));

        if (chunk != chunk_null)
        {
          if (buf_.capacity () < size_ + n)
            buf_.capacity (size_ + n, size_);

          memcpy (buf_.data () + size_, buf, n);
          size_ += n;
        }

        if (chunk == chunk_one ||
            chunk == chunk_last ||
            chunk == chunk_null)
          break;
      }
    }
  }
}
