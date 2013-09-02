// file      : odb/pgsql/traits-calls.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_TRAITS_CALLS_HXX
#define ODB_PGSQL_TRAITS_CALLS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/schema-version.hxx>
#include <odb/traits.hxx>

#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/pgsql-types.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename T,
              bool versioned = object_traits_impl<T, id_pgsql>::versioned>
    struct traits_calls;

    template <typename T>
    struct traits_calls<T, false>
    {
      typedef object_traits_impl<T, id_pgsql> traits;
      typedef typename traits::image_type image_type;
      typedef pgsql::bind bind_type;

      traits_calls (const schema_version_migration*) {}

      const schema_version_migration*
      version () const {return 0;}

      static bool
      grow (image_type& i, bool* t)
      {
        return traits::grow (i, t);
      }

      static void
      bind (bind_type* b, image_type& i, statement_kind sk)
      {
        traits::bind (b, i, sk);
      }

      // Poly-derived version.
      //
      static void
      bind (bind_type* b,
            const bind_type* id, std::size_t id_size,
            image_type& i,
            statement_kind sk)
      {
        traits::bind (b, id, id_size, i, sk);
      }

      static void
      init (T& o, const image_type& i, odb::database* db)
      {
        traits::init (o, i, db);
      }

      static bool
      find_ (typename traits::statements_type& sts,
             const typename traits::id_type* id)
      {
        return traits::find_ (sts, id);
      }
    };

    template <typename T>
    struct traits_calls<T, true>
    {
      typedef object_traits_impl<T, id_pgsql> traits;
      typedef typename traits::image_type image_type;
      typedef pgsql::bind bind_type;

      traits_calls (const schema_version_migration* svm): svm_ (*svm) {}

      const schema_version_migration*
      version () const {return &svm_;}

      bool
      grow (image_type& i, bool* t) const
      {
        return traits::grow (i, t, svm_);
      }

      void
      bind (bind_type* b, image_type& i, statement_kind sk) const
      {
        traits::bind (b, i, sk, svm_);
      }

      // Poly-derived version.
      //
      void
      bind (bind_type* b,
            const bind_type* id, std::size_t id_size,
            image_type& i,
            statement_kind sk) const
      {
        traits::bind (b, id, id_size, i, sk, svm_);
      }

      void
      init (T& o, const image_type& i, odb::database* db) const
      {
        traits::init (o, i, db, svm_);
      }

      bool
      find_ (typename traits::statements_type& sts,
             const typename traits::id_type* id) const
      {
        return traits::find_ (sts, id, svm_);
      }

    private:
      const schema_version_migration& svm_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_TRAITS_CALLS_HXX
