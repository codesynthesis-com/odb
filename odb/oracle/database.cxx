// file      : odb/oracle/database.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <sstream>

#include <odb/oracle/database.hxx>
#include <odb/oracle/exceptions.hxx>
#include <odb/oracle/error.hxx>

#include <odb/oracle/details/options.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    using odb::details::transfer_ptr;

    database::
    database (const string& user,
              const string& password,
              const string& db,
              ub2 charset,
              ub2 ncharset,
              OCIEnv* environment,
              transfer_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          db_ (db),
          port_ (0),
          charset_ (charset),
          ncharset_ (ncharset_),
          environment_ (environment),
          factory_ (factory.transfer ())
    {
      if (environment_ == 0)
      {
        sword s (OCIEnvNlsCreate (&environment_,
                                  OCI_THREADED,
                                  0, 0, 0, 0, 0, 0,
                                  charset,
                                  ncharset));

        if (s == OCI_ERROR)
          translate_error (environment_);

        auto_environment_.reset (environment_);
      }

      if (factory_.get () == 0)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& user,
              const string& password,
              const string& service,
              const string& host,
              unsigned int port,
              ub2 charset,
              ub2 ncharset,
              OCIEnv* environment,
              transfer_ptr<connection_factory> factory)
        : user_ (user),
          password_ (password),
          service_ (service),
          host_ (host),
          port_ (port),
          charset_ (charset),
          ncharset_ (ncharset),
          environment_ (environment),
          factory_ (factory.transfer ())
    {
      if (environment_ == 0)
      {
        sword s (OCIEnvNlsCreate (&environment_,
                                  OCI_THREADED,
                                  0, 0, 0, 0, 0, 0,
                                  charset,
                                  ncharset));

        if (s == OCI_ERROR)
          translate_error (environment_);

        auto_environment_.reset (environment_);
      }

      ostringstream ss;

      if (!host.empty ())
      {
        ss << "//" << host_;

        if (port != 0)
          ss << ":" << port;
      }

      if (!service_.empty ())
      {
        if (!host.empty ())
          ss << "/" << service_;
        else
          ss << service_;
      }

      db_ = ss.str ();

      if (factory_.get () == 0)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (int& argc,
              char* argv[],
              bool erase,
              ub2 charset,
              ub2 ncharset,
              OCIEnv* environment,
              transfer_ptr<connection_factory> factory)
        : port_ (0),
          charset_ (charset),
          ncharset_ (ncharset),
          environment_ (environment),
          factory_ (factory.transfer ())
    {
      if (environment_ == 0)
      {
        sword s (OCIEnvNlsCreate (&environment_,
                                  OCI_THREADED,
                                  0, 0, 0, 0, 0, 0,
                                  charset,
                                  ncharset));

        if (s == OCI_ERROR)
          translate_error (environment_);

        auto_environment_.reset (environment_);
      }

      using namespace details;

      try
      {
        cli::argv_file_scanner scan (argc, argv, "--options-file", erase);
        options ops (scan, cli::unknown_mode::skip, cli::unknown_mode::skip);

        if (ops.user_specified ())
          user_ = ops.user ();

        if (ops.password_specified ())
          password_ = ops.password ();

        if (ops.database_specified ())
        {
          if (ops.host_specified () ||
              ops.port_specified () ||
              ops.service_specified ())

            throw cli_exception ("--host, --port, or --service options "
                                 "cannot be specified together with "
                                 "--database option");
          db_ = ops.database ();
        }
        else
        {
          bool host_present (false);
          ostringstream oss;

          if (ops.host_specified () && !ops.host ().empty ())
          {
            host_present = true;

            host_ = ops.host ();
            oss << "//" << host_;

            if (ops.port_specified ())
            {
              port_ = ops.port ();

              if (port_ != 0)
                oss << ":" << port_;
            }
          }

          if (ops.service_specified () && !ops.service ().empty ())
          {
            service_ = ops.service ();

            if (host_present)
              oss << "/" << service_;
            else
              oss << service_;
          }

          db_ = oss.str ();
        }
      }
      catch (const cli::exception& e)
      {
        ostringstream oss;
        oss << e;
        throw cli_exception (oss.str ());
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
