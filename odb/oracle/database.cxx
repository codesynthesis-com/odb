// file      : odb/oracle/database.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <sstream>

#include <odb/oracle/traits.hxx>
#include <odb/oracle/database.hxx>
#include <odb/oracle/statement.hxx>
#include <odb/oracle/transaction.hxx>
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
        : odb::database (id_oracle),
          user_ (user),
          password_ (password),
          db_ (db),
          port_ (0),
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

      if (!factory_)
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
        : odb::database (id_oracle),
          user_ (user),
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

      if (!factory_)
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
        : odb::database (id_oracle),
          port_ (0),
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

      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    void database::
    print_usage (ostream& os)
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

    const database::schema_version_info& database::
    load_schema_version (const string& name) const
    {
      schema_version_info& svi (schema_version_map_[name]);

      // Construct the SELECT statement text.
      //
      string text ("SELECT \"version\", \"migration\" FROM ");

      if (!svi.version_table.empty ())
        text += svi.version_table; // Already quoted.
      else if (!schema_version_table_.empty ())
        text += schema_version_table_; // Already quoted.
      else
        text += "\"schema_version\"";

      text += " WHERE \"name\" = :1";

      // Bind parameters and results. If the schema name is empty, replace
      // it with a single space to workaround the VARCHAR2 empty/NULL issue.
      //
      string n (name.empty () ? string (" ") : name);
      ub2 psize[1] = {static_cast<ub2> (n.size ())};
      sb2 pind[1] = {0};
      bind pbind[1] = {{bind::string,
                        const_cast<char*> (n.c_str ()),
                        &psize[0],
                        psize[0],
                        &pind[0],
                        0}};
      binding param (pbind, 1);
      param.version++;

      char version[12];
      unsigned int migration;
      ub2 rsize[1];
      sb2 rind[2];
      bind rbind[2] = {
        {bind::number,
         version,
         &rsize[0],
         static_cast<ub4> (sizeof (version)),
         &rind[0],
         0},

        {bind::uinteger, &migration, 0, 4, &rind[1], 0}
      };
      binding result (rbind, 2);
      result.version++;

      // If we are not in transaction, then OCI will start an implicit one
      // but only if we try to modify anything. Since our statement is read-
      // only, we can run without a transaction.
      //
      connection_ptr cp;
      if (!transaction::has_current ())
        cp = factory_->connect ();

      oracle::connection& c (
        cp != 0 ? *cp : transaction::current ().connection ());

      try
      {
        select_statement st (c,
                             text,
                             false, // Don't process.
                             false, // Don't optimize.
                             param,
                             result);
        st.execute ();
        auto_result ar (st);

        switch (st.fetch ())
        {
        case select_statement::success:
          {
            value_traits<unsigned long long, id_big_int>::set_value (
              svi.version, version, rsize[0], rind[0] == -1);
            svi.migration = migration != 0;
            assert (st.fetch () == select_statement::no_data);
            break;
          }
        case select_statement::no_data:
          {
            svi.version = 0; // No schema.
            break;
          }
        }
      }
      catch (const database_exception& e)
      {
        // Detect the case where there is no version table.
        //
        if (e.size () != 0 && e.begin ()->error () == 942)
          svi.version = 0; // No schema.
        else
          throw;
      }

      return svi;
    }
  }
}
