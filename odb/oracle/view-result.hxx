// file      : odb/oracle/view-result.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_VIEW_RESULT_HXX
#define ODB_ORACLE_VIEW_RESULT_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/schema-version.hxx>
#include <odb/view-result.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx> // query_base, view_statements
#include <odb/oracle/statement.hxx>
#include <odb/oracle/traits-calls.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T>
    class view_result_impl: public odb::view_result_impl<T>
    {
    public:
      typedef odb::view_result_impl<T> base_type;

      typedef typename base_type::view_type view_type;
      typedef typename base_type::pointer_type pointer_type;

      typedef view_traits_impl<view_type, id_oracle> view_traits;
      typedef typename base_type::pointer_traits pointer_traits;

      typedef view_statements<view_type> statements_type;

      virtual
      ~view_result_impl ();

      view_result_impl (const query_base&,
                        details::shared_ptr<select_statement>,
                        statements_type&,
                        const schema_version_migration*);

      virtual void
      load (view_type&);

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
      view_traits_calls<view_type> tc_;
      bool use_copy_;
      typename view_traits::image_type* image_copy_;
    };
  }
}

#include <odb/oracle/view-result.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_VIEW_RESULT_HXX
