// file      : odb/mssql/object-result.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_OBJECT_RESULT_HXX
#define ODB_MSSQL_OBJECT_RESULT_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/details/shared-ptr.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx> // query, object_statements
#include <odb/mssql/result.hxx>
#include <odb/mssql/statement.hxx>

namespace odb
{
  namespace mssql
  {
    template <typename T>
    class object_result_impl: public odb::object_result_impl<T>
    {
    public:
      typedef odb::object_result_impl<T> base_type;

      typedef typename base_type::object_type object_type;
      typedef typename base_type::object_traits object_traits;
      typedef typename base_type::id_type id_type;

      typedef typename base_type::pointer_type pointer_type;
      typedef typename base_type::pointer_traits pointer_traits;

      virtual
      ~object_result_impl ();

      object_result_impl (const query&,
                          details::shared_ptr<select_statement>,
                          object_statements<object_type>&);

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

      using base_type::current;

    private:
      typedef mssql::change_callback change_callback_type;

      static void
      change_callback (void* context);

    private:
      details::shared_ptr<select_statement> statement_;
      object_statements<object_type>& statements_;
      bool can_load_;
      bool use_copy_;
      typename object_traits::image_type* image_copy_;
    };

    template <typename T>
    class object_result_impl_no_id: public odb::object_result_impl_no_id<T>
    {
    public:
      typedef odb::object_result_impl_no_id<T> base_type;

      typedef typename base_type::object_type object_type;
      typedef typename base_type::object_traits object_traits;

      typedef typename base_type::pointer_type pointer_type;
      typedef typename base_type::pointer_traits pointer_traits;

      virtual
      ~object_result_impl_no_id ();

      object_result_impl_no_id (const query&,
                                details::shared_ptr<select_statement>,
                                object_statements_no_id<object_type>&);

      virtual void
      load (object_type&);

      virtual void
      next ();

      virtual void
      cache ();

      virtual std::size_t
      size ();

      using base_type::current;

    private:
      typedef mssql::change_callback change_callback_type;

      static void
      change_callback (void* context);

    private:
      details::shared_ptr<select_statement> statement_;
      object_statements_no_id<object_type>& statements_;
      bool can_load_;
      bool use_copy_;
      typename object_traits::image_type* image_copy_;
    };
  }
}

#include <odb/mssql/object-result.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_OBJECT_RESULT_HXX
