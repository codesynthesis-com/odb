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
          parameter_offsets_ (q.parameter_offsets_),
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
        parameter_offsets_ = q.parameter_offsets_;
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
      size_t cur_pos (0);

      // Append the argument clause to the clause of this instance.
      //
      for (size_t i (0),
             p (parameters_.size () + 1),
             e (q.parameter_offsets_.size ());
           i < e;
           ++i, ++p)
      {
        size_t n (clause_.size ());

        if (n != 0 && clause_[n - 1] != ' ' && q.clause_[cur_pos] != ' ')
          clause_ += ' ';

        parameter_offset o (q.parameter_offsets_[i]);

        // Append all characters up to the start of the current
        // parameter specifier, including the '$'.
        //
        clause_.append (q.clause_, cur_pos, o.first - cur_pos + 1);

        // Advance current position in source clause to 1 element past
        // the current parameter specifier.
        //
        cur_pos = o.second;

        // Insert the correct parameter number and update the parameter
        // offset to describe its offset in the new clause.
        //
        o.first = clause_.size () - 1;
        ostringstream os;
        os << p;
        clause_.append (os.str ());
        o.second = clause_.size ();

        parameter_offsets_.push_back (o);
      }

      // Copy any remaining characters in q.clause_.
      //
      if (cur_pos < q.clause_.length ())
      {
        size_t n (clause_.size ());

        if (n != 0 && clause_[n - 1] != ' ' && q.clause_[cur_pos] != ' ')
          clause_ += ' ';

        clause_.append (q.clause_, cur_pos, string::npos);
      }

      size_t n = bind_.size ();

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
    add (details::shared_ptr<query_param> p)
    {
      size_t n (clause_.size ());

      if (n != 0 && clause_[n - 1] != ' ')
        clause_ += ' ';

      parameter_offset o;
      o.first = clause_.length ();

      ostringstream ss;
      ss << parameters_.size () + 1;
      clause_ += '$' + ss.str ();

      o.second = clause_.length ();
      parameter_offsets_.push_back (o);

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

      bool inc_ver (false);
      binding& r (binding_);
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

      statement::bind_param (native_binding_, binding_);
      return native_binding_;
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
