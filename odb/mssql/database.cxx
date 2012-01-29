// file      : odb/mssql/database.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::strcmp, std::strncmp
#include <sstream>

#include <odb/mssql/mssql.hxx>
#include <odb/mssql/database.hxx>
#include <odb/mssql/exceptions.hxx>
#include <odb/mssql/error.hxx>

#include <odb/mssql/details/options.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    database::
    database (const std::string& user,
              const std::string& password,
              const std::string& db,
              const std::string& server,
              const std::string& driver,
              const std::string& extra_connect_string,
              SQLHENV environment,
              std::auto_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          db_ (db),
          protocol_ (protocol_auto),
          port_ (0),
          server_ (server),
          driver_ (driver),
          extra_connect_string_ (extra_connect_string),
          environment_ (environment),
          factory_ (factory)
    {
      init ();
    }

    database::
    database (const std::string& user,
              const std::string& password,
              const std::string& db,
              protocol_type protocol,
              const std::string& host,
              const std::string& instance,
              const std::string& driver,
              const std::string& extra_connect_string,
              SQLHENV environment,
              std::auto_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          db_ (db),
          protocol_ (protocol),
          host_ (host),
          instance_ (instance),
          port_ (0),
          driver_ (driver),
          extra_connect_string_ (extra_connect_string),
          environment_ (environment),
          factory_ (factory)
    {
      init ();
    }

    database::
    database (const std::string& user,
              const std::string& password,
              const std::string& db,
              const std::string& host,
              unsigned int port,
              const std::string& driver,
              const std::string& extra_connect_string,
              SQLHENV environment,
              std::auto_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          db_ (db),
          protocol_ (protocol_tcp),
          host_ (host),
          port_ (port),
          driver_ (driver),
          extra_connect_string_ (extra_connect_string),
          environment_ (environment),
          factory_ (factory)
    {
      init ();
    }

    database::
    database (const string& connect_string,
              SQLHENV environment,
              auto_ptr<connection_factory> factory)
        : protocol_ (protocol_auto),
          port_ (0),
          connect_string_ (connect_string),
          environment_ (environment),
          factory_ (factory)
    {
      init ();
    }

    database::
    database (int& argc,
              char* argv[],
              bool erase,
              const std::string& extra_connect_string,
              SQLHENV environment,
              auto_ptr<connection_factory> factory)
        : protocol_ (protocol_auto),
          port_ (0),
          extra_connect_string_ (extra_connect_string),
          environment_ (environment),
          factory_ (factory)
    {
      using namespace details;

      try
      {
        cli::argv_file_scanner scan (argc, argv, "--options-file", erase);
        options ops (scan, cli::unknown_mode::skip, cli::unknown_mode::skip);

        user_ = ops.user ();
        password_ = ops.password ();
        db_ = ops.database ();
        server_ = ops.server ();
        driver_ = ops.driver ();
      }
      catch (const cli::exception& e)
      {
        ostringstream oss;
        oss << e;
        throw cli_exception (oss.str ());
      }

      init ();
    }

    /*

    NOTE: This code hasn't been tested.

    void database::
    parse ()
    {
      // Parse the server string and extract individual parts (protocol,
      // host, instance, and port).
      //
      string port;

      if (server_.compare (0, 4, "lpc:") == 0)
      {
        // lpc:<host>[\<instance>]
        //
        protocol_ = protocol_shm;
        string::size_type p (server_.find (4, '\\'));

        if (p == string::npos)
          host_.assign (server_, 4, string::npos);
        else
        {
          host_.assign (server_, 4, p - 4);
          instance_.assign (server_, p + 1, string::npos);
        }
      }
      else if (server_.compare (0, 3, "np:") == 0)
      {
        // np:<host>\pipe\[MSSQL$<instance>\]sql\query
        //
        protocol_ = protocol_pipe;

        string::size_type p (server_.find (3, '\\'));

        if (p != string::npos)
        {
          host_.assign (server_, 3, p - 3);

          p = server_.find (p + 1, '$');

          if (p != string::npos)
          {
            p++;
            instance_.assign (server_, p, server_.find (p, '\\') - p);
          }
        }
      }
      else
      {
        // <host>[\<instance>][,<port>]
        // tcp:<host>[\<instance>][,<port>]
        //
        string::size_type p1 (0), p2;

        if (server_.compare (0, 4, "tcp:") == 0)
        {
          protocol_ = protocol_tcp;
          p1 = 4;
        }

        p2 = server_.find (p1, '\\');

        if (p2 == string::npos)
        {
          p2 = server_.find (p1, ',');

          if (p2 == string::npos)
            host_.assign (server_, p1, string::npos);
          else
          {
            host_.assign (server_, 4, p2 - p1);
            port.assign (server_, p2 + 1, string::npos);
          }
        }
        else
        {
          host_.assign (server_, 4, p2 - p1);

          p1 = server_.find (p2 + 1, ',');

          if (p1 == string::npos)
            instance_.assign (server_, p2 + 1, string::npos);
          else
          {
            instance_.assign (server_, p2 + 1, p1 - p2 - 1);
            port.assign (server_, p1 + 1, string::npos);
          }
        }
      }

      if (!port.empty ())
      {
        istringstream is (port);
        is >> port;
        protocol_ = protocol_tcp;
      }
    }
    */

    void database::
    init ()
    {
      SQLRETURN r;

      if (environment_ == 0)
      {
        r = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &environment_);

        if (!SQL_SUCCEEDED (r))
          throw database_exception (
            0, "?????", "unable to allocate environment handle");

        auto_environment_.reset (environment_);

        // Set ODBC version.
        //
        r = SQLSetEnvAttr (environment_,
                           SQL_ATTR_ODBC_VERSION,
                           (SQLPOINTER) SQL_OV_ODBC3,
                           0);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, environment_, SQL_HANDLE_ENV);
      }

      // Build the connection string.
      //
      if (connect_string_.empty ())
      {
        // Find the driver.
        //
        if (driver_.empty ())
        {
          for (bool first (true);; )
          {
            char desc[256];
            SQLSMALLINT desc_size, attr_size;

            r = SQLDriversA (environment_,
                             first ? SQL_FETCH_FIRST : SQL_FETCH_NEXT,
                             (SQLCHAR*) desc,
                             sizeof (desc),
                             &desc_size,
                             0,
                             0,
                             &attr_size);

            if (r == SQL_NO_DATA)
              break;
            else if (!SQL_SUCCEEDED (r))
              translate_error (r, environment_, SQL_HANDLE_ENV);

            // Native Client 9.0 (first version).
            //
            if (strcmp (desc, "SQL Native Client") == 0 ||
                strncmp (desc, "SQL Server Native Client", 24) == 0)
            {
              // Compare driver strings lexicographically. Provided that
              // Microsoft keeps its naming consistent, we will get the
              // correct result. For example, "SQL Server Native Client
              // 10.0" (SQL Server 2008) will be greater than "SQL Native
              // Client" (SQL Server 2005). Similarly, "SQL Server Native
              // Client 11.0" (SQL Server 2012) will be preferred over
              // "SQL Server Native Client 10.0" (SQL Server 2008).
              //
              if (desc > driver_)
                driver_ = desc;
            }

            if (first)
              first = false;
          }
        }

        connect_string_ += "DRIVER={";
        connect_string_ += driver_;
        connect_string_ += "};";

        // If necessary, assemble the server address string, depending
        // on which protocol we are using.
        if (server_.empty ())
        {
          switch (protocol_)
          {
          case protocol_auto:
            {
              server_ = (host_.empty () ? "localhost" : host_.c_str ());

              if (!instance_.empty ())
              {
                server_ += '\\';
                server_ += instance_;
              }

              break;
            }
          case protocol_tcp:
            {
              server_ = "tcp:";
              server_ += (host_.empty () ? "localhost" : host_.c_str ());

              // Port seems to take precedence over instance. For example,
              // if you specify both, and the instance name is invalid, the
              // Native Client driver still connects without any problems.
              //
              if (port_ != 0)
              {
                ostringstream os;
                os << port_;
                server_ += ',';
                server_ += os.str ();
              }
              else if (!instance_.empty ())
              {
                server_ += '\\';
                server_ += instance_;
              }

              break;
            }
          case protocol_lpc:
            {
              server_ = "lpc:";
              server_ += (host_.empty () ? "localhost" : host_.c_str ());

              if (!instance_.empty ())
              {
                server_ += '\\';
                server_ += instance_;
              }

              break;
            }
          case protocol_np:
            {
              server_ = "np:\\\\";
              server_ += (host_.empty () ? "." : host_.c_str ());
              server_ += "\\pipe\\";

              if (!instance_.empty ())
              {
                server_ += "MSSQL$";
                server_ += instance_;
                server_ += '\\';
              }

              server_ += "sql\\query";
              break;
            }
          }
        }

        // The Address attribute seems to be preferred over SERVER. However,
        // SQL Server 2005 Native Client only seem to support Address since
        // SP1. Since we don't know the exact driver version, for now always
        // use SERVER with SQL Server 2005 driver.
        //
        connect_string_ += (driver_ == "SQL Native Client"
                            ? "SERVER={"
                            : "Address={");

        connect_string_ += server_;
        connect_string_ += "};";

        // Add login information.
        //
        if (user_.empty ())
          // Windows authentication.
          //
          connect_string_ += "Trusted_Connection=yes;";
        else
        {
          connect_string_ += "UID={";
          connect_string_ += user_;
          connect_string_ += "};";

          if (!password_.empty ())
          {
            connect_string_ += "PWD={";
            connect_string_ += password_;
            connect_string_ += "};";
          }
        }

        // Add database.
        //
        if (!db_.empty ())
        {
          connect_string_ += "Database={";
          connect_string_ += db_;
          connect_string_ += "};";
        }

        // Add any extra connection attributes.
        //
        if (!extra_connect_string_.empty ())
          connect_string_ += extra_connect_string_;
      }

      if (factory_.get () == 0)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    void database::
    print_usage (std::ostream& os)
    {
      details::options::print_usage (os);
    }

    database::
    ~database ()
    {
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
  }
}
