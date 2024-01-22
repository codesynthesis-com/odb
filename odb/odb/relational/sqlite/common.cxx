// file      : odb/relational/sqlite/common.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>

#include <odb/relational/sqlite/common.hxx>

using namespace std;

namespace relational
{
  namespace sqlite
  {
    //
    // member_base
    //

    sql_type const& member_base::
    member_sql_type (semantics::data_member& m)
    {
      return parse_sql_type (column_type (m, key_prefix_), m);
    }

    void member_base::
    traverse_simple (member_info& mi)
    {
      switch (mi.st->type)
      {
      case sql_type::INTEGER:
        {
          traverse_integer (mi);
          break;
        }
      case sql_type::REAL:
        {
          traverse_real (mi);
          break;
        }
      case sql_type::TEXT:
        {
          if (mi.st->stream)
            traverse_text_stream (mi);
          else
            traverse_text (mi);
          break;
        }
      case sql_type::BLOB:
        {
          if (mi.st->stream)
            traverse_blob_stream (mi);
          else
            traverse_blob (mi);
          break;
        }
      case sql_type::invalid:
        {
          assert (false);
          break;
        }
      }
    }

    //
    // member_image_type
    //

    member_image_type::
    member_image_type (base const& x)
        : member_base::base (x), // virtual base
          base (x) {}

    member_image_type::
    member_image_type ()
        : relational::member_base (0, 0, string (), string ()) {}

    member_image_type::
    member_image_type (semantics::type* type,
                       const custom_cxx_type* ct,
                       string const& fq_type,
                       string const& key_prefix)
        : relational::member_base (type, ct, fq_type, key_prefix) {}

    string member_image_type::
    image_type (semantics::data_member& m)
    {
      type_.clear ();
      member_base::traverse (m, true);
      return type_;
    }

    void member_image_type::
    traverse_composite (member_info& mi)
    {
      type_ = "composite_value_traits< " + mi.fq_type () +
        ", id_sqlite >::image_type";
    }

    void member_image_type::
    traverse_integer (member_info&)
    {
      type_ = "long long";
    }

    void member_image_type::
    traverse_real (member_info&)
    {
      type_ = "double";
    }

    void member_image_type::
    traverse_string (member_info&)
    {
      type_ = "details::buffer";
    }

    void member_image_type::
    traverse_stream (member_info&)
    {
      type_ = "sqlite::stream_buffers";
    }

    entry<member_image_type> member_image_type_;

    //
    // member_database_type
    //

    member_database_type_id::
    member_database_type_id (base const& x)
        : member_base::base (x), // virtual base
          base (x) {}

    member_database_type_id::
    member_database_type_id ()
        : member_base::base (0, 0, string (), string ()), // virtual base
          base (0, 0, string (), string ()) {}

    member_database_type_id::
    member_database_type_id (semantics::type* type,
                             const custom_cxx_type* ct,
                             string const& fq_type,
                             string const& key_prefix)
        : member_base::base (type, ct, fq_type, key_prefix), // virtual base
          base (type, ct, fq_type, key_prefix) {}

    string member_database_type_id::
    database_type_id (type& m)
    {
      type_id_.clear ();
      member_base::traverse (m, true);
      return type_id_;
    }

    void member_database_type_id::
    traverse_composite (member_info&)
    {
      assert (false);
    }

    void member_database_type_id::
    traverse_integer (member_info&)
    {
      type_id_ = "sqlite::id_integer";
    }

    void member_database_type_id::
    traverse_real (member_info&)
    {
      type_id_ = "sqlite::id_real";
    }

    void member_database_type_id::
    traverse_text (member_info&)
    {
      type_id_ = "sqlite::id_text";
    }

    void member_database_type_id::
    traverse_blob (member_info&)
    {
      type_id_ = "sqlite::id_blob";
    }

    void member_database_type_id::
    traverse_text_stream (member_info&)
    {
      type_id_ = "sqlite::id_text_stream";
    }

    void member_database_type_id::
    traverse_blob_stream (member_info&)
    {
      type_id_ = "sqlite::id_blob_stream";
    }

    entry<member_database_type_id> member_database_type_id_;

    //
    // query_columns
    //

    struct query_columns: relational::query_columns, context
    {
      query_columns (base const& x): base_impl (x) {}

      virtual string
      database_type_id (semantics::data_member& m)
      {
        return member_database_type_id_.database_type_id (m);
      }

    private:
      member_database_type_id member_database_type_id_;
    };
    entry<query_columns> query_columns_;
  }
}
