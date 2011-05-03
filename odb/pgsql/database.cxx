// file      : odb/pgsql/database.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sstream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/exceptions.hxx>

#include <odb/pgsql/details/options.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    database::
    database (const string& user,
              const string& password,
              const string& db,
              const string& host,
              unsigned int port,
              const string& extra_conninfo,
              auto_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          db_ (db),
          host_ (host),
          port_ (port),
          extra_conninfo_ (extra_conninfo),
          factory_ (factory)
    {
      ostringstream ss;

      if (!user.empty ())
        ss << "user='" << user << "' ";

      if (!password.empty ())
        ss << "password='" << password << "' ";

      if (!db.empty ())
        ss << "dbname='" << db << "' ";

      if (!host.empty ())
        ss << "host='" << host << "' ";

      if (port)
        ss << "port=" << port << " ";

      // Only the last occurence of keyword/value pair is used by libpq.
      // extra_conninfo specified options take precedence.
      //
      if (!extra_conninfo.empty ())
        ss << extra_conninfo;

      conninfo_ = ss.str ();

      // @@ Uncomment once factory has been implemented.
      //
      // if (factory_.get () == 0)
      //   factory_.reset (new connection_pool_factory ());

      // factory_->database (*this);
    }

    database::
    database (const string& user,
              const string& password,
              const string& db,
              const string& host,
              const string& socket_ext,
              const string& extra_conninfo,
              auto_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          db_ (db),
          host_ (host),
          port_ (0),
          socket_ext_ (socket_ext),
          extra_conninfo_ (extra_conninfo),
          factory_ (factory)
    {
      ostringstream ss;

      if (!user.empty ())
        ss << "user='" << user << "' ";

      if (!password.empty ())
        ss << "password='" << password << "' ";

      if (!db.empty ())
        ss << "dbname='" << db << "' ";

      if (!host.empty ())
        ss << "host='" << host << "' ";

      if (!socket_ext.empty ())
        ss << "port='" << socket_ext << "' ";

      // Only the last occurence of keyword/value pair is used by libpq.
      // extra_conninfo specified options take precedence.
      //
      if (!extra_conninfo.empty ())
        ss << extra_conninfo;

      conninfo_ = ss.str ();

      // @@ Uncomment once factory has been implemented.
      //
      // if (factory_.get () == 0)
      //   factory_.reset (new connection_pool_factory ());

      // factory_->database (*this);
    }

    database::
    database (const string& conninfo,
              auto_ptr<connection_factory> factory)
        : port_ (0), conninfo_ (conninfo), factory_ (factory)
    {
      // @@ Uncomment once factory has been implemented.
      //
      // if (factory_.get () == 0)
      //   factory_.reset (new connection_pool_factory ());

      // factory_->database (*this);
    }

    database::
    database (int& argc,
              char* argv[],
              bool erase,
              const string& extra_conninfo,
              auto_ptr<connection_factory> factory)
        : port_ (0), factory_ (factory)
    {
      using namespace details;

      try
      {
        cli::argv_file_scanner scan (argc, argv, "--options-file", erase);
        options ops (scan, cli::unknown_mode::skip, cli::unknown_mode::skip);

        ostringstream oss;

        if (ops.user_specified ())
        {
          user_ = ops.user ();
          oss << "user='" << user_ << "' ";
        }

        if (ops.password_specified ())
        {
          password_ = ops.password ();
          oss << "password='" << password_ << "' ";
        }

        if (ops.database_specified ())
        {
          db_ = ops.database ();
          oss << "dbname='" << db_ << "' ";
        }

        if (ops.host_specified ())
        {
          host_ = ops.host ();
          oss << "host='" << host_ << "' ";
        }

        if (ops.port_specified ())
        {
          istringstream iss (ops.port ());

          if (iss >> port_ && iss.eof ())
            oss << " port=" << port_ << " ";
          else
          {
            port_ = 0;
            socket_ext_ = ops.port ();
            oss << "port='" << socket_ext_ << "' ";
          }
        }

        if (!extra_conninfo.empty ())
          oss << extra_conninfo;

        conninfo_ = oss.str ();
      }
      catch (const cli::exception& e)
      {
        ostringstream oss;
        oss << e;
        throw cli_exception (oss.str ());
      }

      // @@ Uncomment once factory has been implemented.
      //
      // if (factory_.get () == 0)
      //   factory_.reset (new connection_pool_factory ());

      // factory_->database (*this);
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

    // @@ Implement on completion of supporting code.
    //
    // unsigned long long database::
    // execute (const char* s, std::size_t n)
    // {
    // }

    // @@ Implement on completion of supporting code.
    //
    // transaction_impl* database::
    // begin ()
    // {
    // }
  }
}
