// file      : odb/relational/mssql/context.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_MSSQL_CONTEXT_HXX
#define ODB_RELATIONAL_MSSQL_CONTEXT_HXX

#include <map>

#include <odb/relational/context.hxx>

namespace relational
{
  namespace mssql
  {
    struct sql_type
    {
      // Keep the order in each block of types.
      //
      enum core_type
      {
        // Integral types.
        //
        BIT,
        TINYINT,
        SMALLINT,
        INT,
        BIGINT,

        // Fixed and floating point types.
        //
        DECIMAL,
        SMALLMONEY,
        MONEY,
        FLOAT,

        // String and binary types.
        //
        CHAR,
        VARCHAR,
        TEXT,

        NCHAR,
        NVARCHAR,
        NTEXT,

        BINARY,
        VARBINARY,
        IMAGE,

        // Date-time types.
        //
        DATE,
        TIME,
        DATETIME,
        DATETIME2,
        SMALLDATETIME,
        DATETIMEOFFSET,

        // Other types.
        //
        UNIQUEIDENTIFIER,
        ROWVERSION,

        // Invalid type.
        //
        invalid
      };

      sql_type () :
          type (invalid),
          has_prec (false), prec (0),
          has_scale (false), scale (0)
      {
      }

      core_type type;

      bool has_prec;
      unsigned short prec;  // Max numeric value is 8000. 0 indicates
                            // 'max' as in VARCHAR(max).
      bool has_scale;
      unsigned short scale; // Max value is 38.

      // Conversion expressions for custom database types.
      //
      std::string to;
      std::string from;
    };

    class context: public virtual relational::context
    {
    public:
      sql_type const&
      parse_sql_type (string const&,
                      semantics::data_member&,
                      bool custom = true);

      // Return true if this type is long data.
      //
      bool
      long_data (sql_type const&);

    public:
      struct invalid_sql_type
      {
        invalid_sql_type (string const& message): message_ (message) {}

        string const&
        message () const {return message_;}

      private:
        string message_;
      };

      // If custom_db_types is NULL, then this function returns
      // invalid type instead of throwing in case an unknown type
      // is encountered.
      //
      static sql_type
      parse_sql_type (string const&, custom_db_types const* = 0);

    protected:
      virtual string const&
      convert_expr (string const&, semantics::data_member&, bool);

      virtual string
      quote_id_impl (qname const&) const;

    protected:
      virtual string
      database_type_impl (semantics::type&, semantics::names*, bool, bool*);

    public:
      virtual
      ~context ();

      context ();
      context (std::ostream&,
               semantics::unit&,
               options_type const&,
               features_type&,
               sema_rel::model*);

      static context&
      current ()
      {
        return *current_;
      }

    private:
      static context* current_;

    private:
      struct data: base_context::data
      {
        data (std::ostream& os): base_context::data (os) {}

        struct sql_type_cache_entry
        {
          sql_type_cache_entry ()
              : custom_cached (false), straight_cached (false) {}

          sql_type const&
          cache_custom (sql_type const& t)
          {
            custom = t;
            custom_cached = true;
            return custom;
          }

          sql_type const&
          cache_straight (sql_type const& t)
          {
            straight = t;
            straight_cached = true;
            return straight;
          }

          sql_type custom;   // With custom mapping.
          sql_type straight; // Without custom mapping.

          bool custom_cached;
          bool straight_cached;
        };

        typedef std::map<string, sql_type_cache_entry> sql_type_cache;
        sql_type_cache sql_type_cache_;
      };
      data* data_;
    };
  }
}

#endif // ODB_RELATIONAL_MSSQL_CONTEXT_HXX
