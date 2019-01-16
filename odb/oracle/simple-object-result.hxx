// file      : odb/oracle/simple-object-result.hxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_SIMPLE_OBJECT_RESULT_HXX
#define ODB_ORACLE_SIMPLE_OBJECT_RESULT_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/schema-version.hxx>
#include <odb/simple-object-result.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx> // query_base
#include <odb/oracle/statement.hxx>
#include <odb/oracle/traits-calls.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T>
    class object_result_impl: public odb::object_result_impl<T>
    {
    public:
      typedef odb::object_result_impl<T> base_type;

      typedef typename base_type::id_type id_type;
      typedef typename base_type::object_type object_type;
      typedef typename base_type::pointer_type pointer_type;

      typedef object_traits_impl<object_type, id_oracle> object_traits;
      typedef typename base_type::pointer_traits pointer_traits;

      typedef typename object_traits::statements_type statements_type;

      virtual
      ~object_result_impl ();

      object_result_impl (const query_base&,
                          details::shared_ptr<select_statement>,
                          statements_type&,
                          const schema_version_migration*);

      virtual void
      load (object_type&, bool fetch);

      virtual id_type
      load_id ();

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
      typedef oracle::change_callback change_callback_type;

      static void
      change_callback (void* context);

    private:
      details::shared_ptr<select_statement> statement_;
      statements_type& statements_;
      object_traits_calls<object_type> tc_;
      bool use_copy_;
      typename object_traits::image_type* image_copy_;
    };
  }
}

#include <odb/oracle/simple-object-result.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_SIMPLE_OBJECT_RESULT_HXX
