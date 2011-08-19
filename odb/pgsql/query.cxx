// file      : odb/pgsql/query.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset
#include <cassert>
#include <sstream>

#include <odb/pgsql/query.hxx>
#include <odb/pgsql/statement.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    // query_param
    //
    query_param::
    ~query_param ()
    {
    }

    query::
    query (const query& q)
        : clause_ (q.clause_),
          parameters_ (q.parameters_),
          bind_ (q.bind_),
          binding_ (0, 0),
          values_ (q.values_),
          lengths_ (q.lengths_),
          formats_ (q.formats_),
          types_ (q.types_),
          native_binding_ (0, 0, 0, 0)
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

        native_binding_.values = &values_[0];
        native_binding_.lengths = &lengths_[0];
        native_binding_.formats = &formats_[0];
        native_binding_.count = n;

        assert (values_.size () == n);
        assert (lengths_.size () == n);
        assert (formats_.size () == n);
        assert (types_.size () == n);

        statement::bind_param (native_binding_, binding_);
      }
    }

    query& query::
    operator= (const query& q)
    {
      if (this != &q)
      {
        clause_ = q.clause_;
        parameters_ = q.parameters_;
        bind_ = q.bind_;

        size_t n (bind_.size ());

        binding_.count = n;
        binding_.version++;

        values_ = q.values_;
        lengths_ = q.lengths_;
        formats_ = q.formats_;
        types_ = q.types_;

        native_binding_.count = n;

        assert (values_.size () == n);
        assert (lengths_.size () == n);
        assert (formats_.size () == n);
        assert (types_.size () == n);

        if (n != 0)
        {
          binding_.bind = &bind_[0];

          native_binding_.values = &values_[0];
          native_binding_.lengths = &lengths_[0];
          native_binding_.formats = &formats_[0];

          statement::bind_param (native_binding_, binding_);
        }
      }

      return *this;
    }

    query& query::
    operator+= (const query& q)
    {
      clause_.insert (clause_.end (), q.clause_.begin (), q.clause_.end ());

      size_t n (bind_.size ());

      parameters_.insert (
        parameters_.end (), q.parameters_.begin (), q.parameters_.end ());

      bind_.insert (
        bind_.end (), q.bind_.begin (), q.bind_.end ());

      values_.insert (
        values_.end (), q.values_.begin (), q.values_.end ());

      lengths_.insert (
        lengths_.end (), q.lengths_.begin (), q.lengths_.end ());

      formats_.insert (
        formats_.end (), q.formats_.begin (), q.formats_.end ());

      types_.insert (
        types_.end (), q.types_.begin (), q.types_.end ());

      if (n != bind_.size ())
      {
        n = bind_.size ();

        binding_.bind = &bind_[0];
        binding_.count = n;
        binding_.version++;

        assert (values_.size () == n);
        assert (lengths_.size () == n);
        assert (formats_.size () == n);
        assert (types_.size () == n);

        native_binding_.values = &values_[0];
        native_binding_.lengths = &lengths_[0];
        native_binding_.formats = &formats_[0];
        native_binding_.count = n;

        statement::bind_param (native_binding_, binding_);
      }

      return *this;
    }

    void query::
    append (const string& q, clause_part::kind_type k)
    {
      if (k == clause_part::native &&
          !clause_.empty () &&
          clause_.back ().kind == clause_part::native)
      {
        string& s (clause_.back ().part);

        char first (!q.empty () ? q[0] : ' ');
        char last (!s.empty () ? s[s.size () - 1] : ' ');

        // We don't want extra spaces after '(' as well as before ','
        // and ')'.
        //
        if (last != ' ' && last != '(' &&
            first != ' ' && first != ',' && first != ')')
          s += ' ';

        s += q;
      }
      else
        clause_.push_back (clause_part (k, q));
    }

    void query::
    add (details::shared_ptr<query_param> p)
    {
      clause_.push_back (clause_part (clause_part::param));

      parameters_.push_back (p);
      bind_.push_back (bind ());
      binding_.bind = &bind_[0];
      binding_.count = bind_.size ();
      binding_.version++;

      bind* b (&bind_.back ());
      memset (b, 0, sizeof (bind));
      p->bind (b);

      values_.push_back (0);
      lengths_.push_back (0);
      formats_.push_back (0);
      native_binding_.values = &values_[0];
      native_binding_.lengths = &lengths_[0];
      native_binding_.formats = &formats_[0];

      // native_binding_.count should always equal binding_.count.
      // At this point, we know that we have added one element to
      // each array, so there is no need to check.
      //
      native_binding_.count = binding_.count;

      types_.push_back (p->oid ());

      statement::bind_param (native_binding_, binding_);
    }

    native_binding& query::
    parameters_binding () const
    {
      size_t n (parameters_.size ());

      if (n == 0)
        return native_binding_;

      bool ref (false), inc_ver (false);
      binding& r (binding_);
      bind* b (&bind_[0]);

      for (size_t i (0); i < n; ++i)
      {
        query_param& p (*parameters_[i]);

        if (p.reference ())
        {
          ref = true;

          if (p.init ())
          {
            p.bind (b + i);
            inc_ver = true;
          }
        }
      }

      if (ref)
      {
        statement::bind_param (native_binding_, binding_);

        if (inc_ver)
          r.version++;
      }

      return native_binding_;
    }

    string query::
    clause (string const& table) const
    {
      string r;
      size_t param (1);

      for (clause_type::const_iterator i (clause_.begin ()),
             end (clause_.end ()); i != end; ++i)
      {
        char last (!r.empty () ? r[r.size () - 1] : ' ');

        switch (i->kind)
        {
        case clause_part::column:
          {
            if (last != ' ' && last != '(')
              r += ' ';

            if (i->part[0] == '.')
              r += table;

            r += i->part;
            break;
          }
        case clause_part::param:
          {
            if (last != ' ' && last != '(')
              r += ' ';

            ostringstream os;
            os << param++;
            r += '$';
            r += os.str ();
            break;
          }
        case clause_part::native:
          {
            // We don't want extra spaces after '(' as well as before ','
            // and ')'.
            //
            const string& p (i->part);
            char first (!p.empty () ? p[0] : ' ');

            if (last != ' ' && last != '(' &&
                first != ' ' && first != ',' && first != ')')
              r += ' ';

            r += p;
            break;
          }
        }
      }

      if (r.empty () ||
          r.compare (0, 6, "WHERE ") == 0 ||
          r.compare (0, 9, "ORDER BY ") == 0 ||
          r.compare (0, 9, "GROUP BY ") == 0 ||
          r.compare (0, 7, "HAVING ") == 0)
        return r;
      else
        return "WHERE " + r;
    }
  }
}
