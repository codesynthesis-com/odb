// file      : odb/mssql/polymorphic-object-result.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_POLYMORPHIC_OBJECT_RESULT_HXX
#define ODB_MSSQL_POLYMORPHIC_OBJECT_RESULT_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/schema-version.hxx>
#include <odb/polymorphic-object-result.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx> // query_base
#include <odb/mssql/statement.hxx>
#include <odb/mssql/traits-calls.hxx>

namespace odb
{
  namespace mssql
  {
    template <typename T>
    class polymorphic_object_result_impl:
      public odb::polymorphic_object_result_impl<T>
    {
    public:
      typedef odb::polymorphic_object_result_impl<T> base_type;

      typedef typename base_type::id_type id_type;
      typedef typename base_type::object_type object_type;
      typedef typename base_type::pointer_type pointer_type;

      typedef object_traits_impl<object_type, id_mssql> object_traits;
      typedef typename base_type::pointer_traits pointer_traits;

      typedef typename base_type::root_type root_type;
      typedef typename base_type::discriminator_type discriminator_type;

      typedef object_traits_impl<root_type, id_mssql> root_traits;

      typedef typename object_traits::image_type image_type;
      typedef typename object_traits::statements_type statements_type;

      virtual
      ~polymorphic_object_result_impl ();

      polymorphic_object_result_impl (const query_base&,
                                      details::shared_ptr<select_statement>,
                                      statements_type&,
                                      const schema_version_migration*);

      virtual void
      load (object_type*, bool fetch);

      virtual id_type
      load_id ();

      virtual discriminator_type
      load_discriminator ();

      virtual void
      next ();

      virtual void
      cache ();

      virtual std::size_t
      size ();

      virtual void
      invalidate ();

      using base_type::current;

    private:
      typedef mssql::change_callback change_callback_type;

      static void
      change_callback (void* context);

    private:
      details::shared_ptr<select_statement> statement_;
      statements_type& statements_;
      object_traits_calls<object_type> tc_;
      bool can_load_;
      bool use_copy_;
      image_type* image_copy_;
    };
  }
}

#include <odb/mssql/polymorphic-object-result.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_POLYMORPHIC_OBJECT_RESULT_HXX
