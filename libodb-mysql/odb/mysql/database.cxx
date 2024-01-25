// file      : odb/mysql/database.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sstream>
#include <cstring> // std::memset

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/database.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/connection-factory.hxx>
#include <odb/mysql/transaction.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/exceptions.hxx>

#include <odb/mysql/details/options.hxx>

using namespace std;

namespace odb
{
  namespace mysql
  {
    using odb::details::transfer_ptr;

    database::
    ~database ()
    {
    }

    database::
    database (const char* user,
              const char* passwd,
              const char* db,
              const char* host,
              unsigned int port,
              const char* socket,
              const char* charset,
              unsigned long client_flags,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_mysql),
          user_ (user ? user : ""),
          passwd_str_ (passwd ? passwd : ""),
          passwd_ (passwd ? passwd_str_.c_str () : 0),
          db_ (db ? db : ""),
          host_ (host ? host : ""),
          port_ (port),
          socket_str_ (socket ? socket : ""),
          socket_ (socket ? socket_str_.c_str () : 0),
          charset_ (charset == 0 ? "" : charset),
          client_flags_ (client_flags),
          factory_ (factory.transfer ())
    {
      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& user,
              const string& passwd,
              const string& db,
              const string& host,
              unsigned int port,
              const string* socket,
              const string& charset,
              unsigned long client_flags,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_mysql),
          user_ (user),
          passwd_str_ (passwd),
          passwd_ (passwd_str_.c_str ()),
          db_ (db),
          host_ (host),
          port_ (port),
          socket_str_ (socket ? *socket : ""),
          socket_ (socket ? socket_str_.c_str () : 0),
          charset_ (charset),
          client_flags_ (client_flags),
          factory_ (factory.transfer ())
    {
      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& user,
              const string* passwd,
              const string& db,
              const string& host,
              unsigned int port,
              const string* socket,
              const string& charset,
              unsigned long client_flags,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_mysql),
          user_ (user),
          passwd_str_ (passwd ? *passwd : ""),
          passwd_ (passwd ? passwd_str_.c_str () : 0),
          db_ (db),
          host_ (host),
          port_ (port),
          socket_str_ (socket ? *socket : ""),
          socket_ (socket ? socket_str_.c_str () : 0),
          charset_ (charset),
          client_flags_ (client_flags),
          factory_ (factory.transfer ())
    {
      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& user,
              const string& passwd,
              const string& db,
              const string& host,
              unsigned int port,
              const string& socket,
              const string& charset,
              unsigned long client_flags,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_mysql),
          user_ (user),
          passwd_str_ (passwd),
          passwd_ (passwd_str_.c_str ()),
          db_ (db),
          host_ (host),
          port_ (port),
          socket_str_ (socket),
          socket_ (socket_str_.c_str ()),
          charset_ (charset),
          client_flags_ (client_flags),
          factory_ (factory.transfer ())
    {
      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& user,
              const string* passwd,
              const string& db,
              const string& host,
              unsigned int port,
              const string& socket,
              const string& charset,
              unsigned long client_flags,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_mysql),
          user_ (user),
          passwd_str_ (passwd ? *passwd : ""),
          passwd_ (passwd ? passwd_str_.c_str () : 0),
          db_ (db),
          host_ (host),
          port_ (port),
          socket_str_ (socket),
          socket_ (socket_str_.c_str ()),
          charset_ (charset),
          client_flags_ (client_flags),
          factory_ (factory.transfer ())
    {
      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (int& argc,
              char* argv[],
              bool erase,
              const string& charset,
              unsigned long client_flags,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_mysql),
          passwd_ (0),
          socket_ (0),
          charset_ (charset),
          client_flags_ (client_flags),
          factory_ (factory.transfer ())
    {
      using namespace details;

      try
      {
        cli::argv_file_scanner scan (argc, argv, "--options-file", erase);
        options ops (scan, cli::unknown_mode::skip, cli::unknown_mode::skip);

        user_ = ops.user ();

        if (ops.password_specified ())
        {
          passwd_str_ = ops.password ();
          passwd_ = passwd_str_.c_str ();
        }

        db_ = ops.database ();
        host_ = ops.host ();
        port_ = ops.port ();

        if (ops.socket_specified ())
        {
          socket_str_ = ops.socket ();
          socket_ = socket_str_.c_str ();
        }
      }
      catch (const cli::exception& e)
      {
        ostringstream ostr;
        ostr << e;
        throw cli_exception (ostr.str ());
      }

      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    void database::
    print_usage (ostream& os)
    {
      details::options::print_usage (os);
    }

    transaction_impl* database::
    begin ()
    {
      return new transaction_impl (*this);
    }

    odb::connection* database::
    connection_ ()
    {
      connection_ptr c (factory_->connect ());
      return c.release ();
    }

    const database::schema_version_info& database::
    load_schema_version (const string& name) const
    {
      schema_version_info& svi (schema_version_map_[name]);

      // Construct the SELECT statement text.
      //
      string text ("SELECT `version`, `migration` FROM ");

      if (!svi.version_table.empty ())
        text += svi.version_table; // Already quoted.
      else if (!schema_version_table_.empty ())
        text += schema_version_table_; // Already quoted.
      else
        text += "`schema_version`";

      text += " WHERE `name` = ?";

      // Bind parameters and results.
      //
      unsigned long psize[1] = {static_cast<unsigned long> (name.size ())};
      my_bool pnull[1] = {0};
      MYSQL_BIND pbind[1];
      binding param (pbind, 1);

      memset (pbind, 0, sizeof (pbind));

      pbind[0].buffer_type = MYSQL_TYPE_STRING;
      pbind[0].buffer = const_cast<char*> (name.c_str ());
      pbind[0].buffer_length = psize[0];
      pbind[0].length = &psize[0];
      pbind[0].is_null = &pnull[0];

      param.version++;

      signed char migration;
      my_bool rnull[2];
      MYSQL_BIND rbind[2];
      binding result (rbind, 2);

      memset (rbind, 0, sizeof (rbind));

      rbind[0].buffer_type = MYSQL_TYPE_LONGLONG;
      rbind[0].is_unsigned = 1;
      rbind[0].buffer = &svi.version;
      rbind[0].is_null = &rnull[0];

      rbind[1].buffer_type = MYSQL_TYPE_TINY;
      rbind[1].is_unsigned = 0;
      rbind[1].buffer = &migration;
      rbind[1].is_null = &rnull[1];

      result.version++;

      // If we are not in transaction, MySQL will use an implicit one
      // (i.e., autocommit mode), which suits us just fine.
      //
      connection_ptr cp;
      if (!transaction::has_current ())
        cp = factory_->connect ();

      mysql::connection& c (
        cp != 0
        ? *cp
        : transaction::current ().connection (const_cast<database&> (*this)));

      try
      {
        select_statement st (c,
                             text.c_str (),
                             false, // Don't process.
                             false, // Don't optimize.
                             param,
                             result,
                             false);
        st.execute ();
        auto_result ar (st);

        switch (st.fetch ())
        {
        case select_statement::success:
          {
            svi.migration = migration != 0;
            assert (st.fetch () == select_statement::no_data);
            break;
          }
        case select_statement::no_data:
          {
            svi.version = 0; // No schema.
            break;
          }
        case select_statement::truncated:
          {
            assert (false);
            break;
          }
        }
      }
      catch (const database_exception& e)
      {
        // Detect the case where there is no version table.
        //
        if (e.error () == ER_NO_SUCH_TABLE)
          svi.version = 0; // No schema.
        else
          throw;
      }

      return svi;
    }
  }
}
