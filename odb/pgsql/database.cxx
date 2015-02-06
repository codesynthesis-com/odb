// file      : odb/pgsql/database.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sstream>

#include <odb/pgsql/traits.hxx>
#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/connection-factory.hxx>
#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/transaction.hxx>
#include <odb/pgsql/exceptions.hxx>

#include <odb/pgsql/details/options.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    using odb::details::transfer_ptr;

    database::
    database (const string& user,
              const string& password,
              const string& db,
              const string& host,
              unsigned int port,
              const string& extra_conninfo,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_pgsql),
          user_ (user),
          password_ (password),
          db_ (db),
          host_ (host),
          port_ (port),
          extra_conninfo_ (extra_conninfo),
          factory_ (factory.transfer ())
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

      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& user,
              const string& password,
              const string& db,
              const string& host,
              const string& socket_ext,
              const string& extra_conninfo,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_pgsql),
          user_ (user),
          password_ (password),
          db_ (db),
          host_ (host),
          port_ (0),
          socket_ext_ (socket_ext),
          extra_conninfo_ (extra_conninfo),
          factory_ (factory.transfer ())
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

      if (!factory_)
        factory_.reset (new connection_pool_factory ());

      factory_->database (*this);
    }

    database::
    database (const string& conninfo, transfer_ptr<connection_factory> factory)
        : odb::database (id_pgsql),
          port_ (0),
          conninfo_ (conninfo),
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
              const string& extra_conninfo,
              transfer_ptr<connection_factory> factory)
        : odb::database (id_pgsql), port_ (0), factory_ (factory.transfer ())
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

      if (!factory_)
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

      text += " WHERE \"name\" = $1";

      // Bind parameters and results.
      //
      size_t psize[1] = {name.size ()};
      bool pnull[1] = {false};
      bind pbind[1] = {{bind::text,
                        const_cast<char*> (name.c_str ()),
                        &psize[0],
                        psize[0],
                        &pnull[0],
                        0}};
      binding param (pbind, 1);
      param.version++;

      unsigned int param_types[1] = {text_oid};

      char* values[1];
      int lengths[1];
      int formats[1];
      native_binding nparam (values, lengths, formats, 1);

      long long version;
      bool rnull[2];
      bind rbind[2] = {{bind::bigint, &version, 0, 0, &rnull[0], 0},
                       {bind::boolean_, &svi.migration, 0, 0, &rnull[1], 0}};
      binding result (rbind, 2);
      result.version++;

      // If we are not in transaction, PostgreSQL will start an implicit one
      // which suits us just fine.
      //
      connection_ptr cp;
      if (!transaction::has_current ())
        cp = factory_->connect ();

      pgsql::connection& c (
        cp != 0 ? *cp : transaction::current ().connection ());

      try
      {
        select_statement st (c,
                             "odb_database_schema_version",
                             text.c_str (),
                             false, // Don't process.
                             false, // Don't optimize.
                             param_types,
                             1,
                             param,
                             nparam,
                             result,
                             false);
        st.execute ();
        auto_result ar (st);

        switch (st.fetch ())
        {
        case select_statement::success:
          {
            value_traits<unsigned long long, id_bigint>::set_value (
              svi.version, version, rnull[0]);
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
        if (e.sqlstate () == "42P01")
          svi.version = 0; // No schema.
        else
          throw;
      }

      return svi;
    }
  }
}
