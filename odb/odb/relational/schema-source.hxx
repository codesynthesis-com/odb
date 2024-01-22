// file      : odb/relational/schema-source.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_SCHEMA_SOURCE_HXX
#define ODB_RELATIONAL_SCHEMA_SOURCE_HXX

#include <odb/diagnostics.hxx>

#include <odb/emitter.hxx>
#include <odb/relational/context.hxx>
#include <odb/relational/schema.hxx>

namespace relational
{
  namespace schema
  {
    struct cxx_emitter: emitter, virtual context
    {
      typedef cxx_emitter base;

      void
      pass (unsigned short p)
      {
        empty_ = true;
        pass_ = p;
        new_pass_ = true;

        if (pass_ == 1)
          empty_passes_ = 0; // New set of passes.

        // Assume this pass is empty.
        //
        empty_passes_++;
      }

      // Did this pass produce anything?
      //
      bool
      empty () const
      {
        return empty_;
      }

      virtual void
      pre ()
      {
        first_ = true;
      }

      virtual void
      line (const string& l)
      {
        if (l.empty ())
          return; // Ignore empty lines.

        if (first_)
        {
          first_ = false;

          // If this line starts a new pass, then output the switch/case
          // blocks.
          //
          if (new_pass_)
          {
            new_pass_ = false;
            empty_ = false;
            empty_passes_--; // This pass is not empty.

            // Output case statements for empty preceeding passes, if any.
            //
            if (empty_passes_ != 0)
            {
              unsigned short s (pass_ - empty_passes_);

              if (s == 1)
                os << "switch (pass)"
                   << "{";
              else
                os << "return true;" // One more pass.
                   << "}";

              for (; s != pass_; ++s)
                os << "case " << s << ":" << endl;

              os << "{";
              empty_passes_ = 0;
            }

            if (pass_ == 1)
              os << "switch (pass)"
                 << "{";
            else
              os << "return true;" // One more pass.
                 << "}";

            os << "case " << pass_ << ":" << endl
               << "{";
          }

          os << "db.execute (";
        }
        else
          os << strlit (line_ + '\n') << endl;

        line_ = l;
      }

      virtual void
      post ()
      {
        if (!first_) // Ignore empty statements.
          os << strlit (line_) << ");";
      }

    private:
      std::string line_;
      bool first_;
      bool empty_;
      bool new_pass_;
      unsigned short pass_;
      unsigned short empty_passes_; // Number of preceding empty passes.
    };
  }
}

#endif // ODB_RELATIONAL_SCHEMA_SOURCE_HXX
