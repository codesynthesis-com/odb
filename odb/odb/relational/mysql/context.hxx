// file      : odb/relational/mysql/context.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_MYSQL_CONTEXT_HXX
#define ODB_RELATIONAL_MYSQL_CONTEXT_HXX

#include <map>
#include <vector>

#include <odb/relational/context.hxx>

namespace relational
{
  namespace mysql
  {
    struct sql_type
    {
      // Keep the order in each block of types.
      //
      enum core_type
      {
        // Integral types.
        //
        TINYINT,
        SMALLINT,
        MEDIUMINT,
        INT,
        BIGINT,

        // Float types.
        //
        FLOAT,
        DOUBLE,
        DECIMAL,

        // Data-time types.
        //
        DATE,
        TIME,
        DATETIME,
        TIMESTAMP,
        YEAR,

        // String and binary types.
        //
        CHAR,
        BINARY,
        VARCHAR,
        VARBINARY,
        TINYTEXT,
        TINYBLOB,
        TEXT,
        BLOB,
        MEDIUMTEXT,
        MEDIUMBLOB,
        LONGTEXT,
        LONGBLOB,

        // Other types.
        //
        BIT,
        ENUM,
        SET,

        // Invalid type.
        //
        invalid
      };

      sql_type () : type (invalid), unsign (false), range (false) {}

      core_type type;
      bool unsign;
      bool range;
      unsigned int range_value; // MySQL max value is 2^32 - 1 (LONGBLOG/TEXT).
      std::vector<std::string> enumerators; // Enumerator strings for ENUM.

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
      parse_sql_type (string, custom_db_types const* = 0);

    protected:
      virtual string const&
      convert_expr (string const&, semantics::data_member&, bool);

      virtual bool
      grow_impl (semantics::class_&, user_section*);

      virtual bool
      grow_impl (semantics::data_member&);

      virtual bool
      grow_impl (semantics::data_member&,
                 semantics::type&,
                 const custom_cxx_type*,
                 string const&);

    protected:
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

#endif // ODB_RELATIONAL_MYSQL_CONTEXT_HXX
