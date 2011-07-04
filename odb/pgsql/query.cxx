// file      : odb/pgsql/query.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cstddef> // std::size_t
#include <cstring> // std::memset
#include <cassert>
#include <sstream>

#include <odb/pgsql/query.hxx>

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
        }
      }

      return *this;
    }

    query& query::
    operator+= (const query& q)
    {
      size_t n (clause_.size ());

      if (n != 0 && clause_[n - 1] != ' ' &&
          !q.clause_.empty () && q.clause_[0] != ' ')
        clause_ += ' ';

      clause_ += q.clause_;

      // Reset parameter indexes.
      //
      bool unquoted (true);
      for (std::size_t i (0), e (clause_.size ()), p(1); i < e; ++i)
      {
        // $'s are legal in identifiers in PostgreSQL.
        //
        if (clause_[i] == '"')
          unquoted = !unquoted;

        if (unquoted && clause_[i] == '$')
        {
          ostringstream ss;
          ss << p++;
          clause_[++i] = ss.str()[0];
        }
      }

      n = bind_.size ();

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
      }

      return *this;
    }

    void query::
    add (details::shared_ptr<query_param> p)
    {
      size_t n (clause_.size ());

      if (n != 0 && clause_[n - 1] != ' ')
        clause_ += ' ';

      ostringstream ss;
      ss << parameters_.size () + 1;

      clause_ += '$' + ss.str ();

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

      types_.push_back (p->oid ());

      // native_binding_.count should always equal binding_.count.
      // At this point, we know that we have added one element to
      // each array, so there is no need to check.
      //
      native_binding_.count = binding_.count;
    }

    binding& query::
    parameters_binding () const
    {
      size_t n (parameters_.size ());
      binding& r (binding_);

      if (n == 0)
        return r;

      bool inc_ver (false);
      bind* b (&bind_[0]);

      for (size_t i (0); i < n; ++i)
      {
        query_param& p (*parameters_[i]);

        if (p.reference ())
        {
          if (p.init ())
          {
            p.bind (b + i);
            inc_ver = true;
          }
        }
      }

      if (inc_ver)
        r.version++;

      return r;
    }

    std::string query::
    clause () const
    {
      if (clause_.empty () ||
          clause_.compare (0, 6, "WHERE ") == 0 ||
          clause_.compare (0, 9, "ORDER BY ") == 0 ||
          clause_.compare (0, 9, "GROUP BY ") == 0 ||
          clause_.compare (0, 7, "HAVING ") == 0)
        return clause_;
      else
        return "WHERE " + clause_;
    }
  }
}
