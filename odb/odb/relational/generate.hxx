// file      : odb/relational/generate.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_GENERATE_HXX
#define ODB_RELATIONAL_GENERATE_HXX

#include <string>
#include <libcutl/shared-ptr.hxx>

#include <odb/context.hxx>
#include <odb/semantics/relational/model.hxx>
#include <odb/semantics/relational/changeset.hxx>
#include <odb/semantics/relational/changelog.hxx>

namespace relational
{
  namespace header
  {
    void
    generate ();
  }

  namespace inline_
  {
    void
    generate ();
  }

  namespace source
  {
    void
    generate ();
  }

  namespace model
  {
    cutl::shared_ptr<semantics::relational::model>
    generate ();
  }

  namespace changelog
  {
    // Returns NULL if the changelog is unchanged.
    //
    cutl::shared_ptr<semantics::relational::changelog>
    generate (semantics::relational::model&,
              model_version const&,
              semantics::relational::changelog* old, // Can be NULL.
              std::string const& in_name,
              std::string const& out_name,
              options const&);
  }

  namespace schema
  {
    void
    generate_prologue ();

    void
    generate_epilogue ();

    void
    generate_drop ();

    void
    generate_create ();

    void
    generate_migrate_pre (semantics::relational::changeset&);

    void
    generate_migrate_post (semantics::relational::changeset&);

    // Generate embedded schema.
    //
    void
    generate_source (semantics::relational::changelog*);
  }
}

#endif // ODB_RELATIONAL_GENERATE_HXX
