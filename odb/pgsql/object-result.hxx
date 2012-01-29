// file      : odb/pgsql/object-result.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_OBJECT_RESULT_HXX
#define ODB_PGSQL_OBJECT_RESULT_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx> // query, object_statements
#include <odb/pgsql/result.hxx>
#include <odb/pgsql/statement.hxx>

namespace odb
{
  namespace pgsql
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
      void
      load_image ();

    private:
      details::shared_ptr<select_statement> statement_;
      object_statements<object_type>& statements_;
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
      details::shared_ptr<select_statement> statement_;
      object_statements_no_id<object_type>& statements_;
    };
  }
}

#include <odb/pgsql/object-result.txx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_OBJECT_RESULT_HXX
