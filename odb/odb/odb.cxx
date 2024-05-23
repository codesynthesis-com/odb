// file      : odb/odb.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <errno.h>
#include <stdlib.h>    // getenv, setenv
#include <string.h>    // strerror, memset
#include <unistd.h>    // stat, close
#include <sys/types.h> // stat
#include <sys/stat.h>  // stat

// Process.
//
#ifndef _WIN32
#  include <unistd.h>    // execvp, fork, dup2, pipe, {STDIN,STDERR}_FILENO
#  include <sys/types.h> // waitpid
#  include <sys/wait.h>  // waitpid
#else
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>   // CreatePipe, CreateProcess, GetTemp*, MAX_PATH
#  include <io.h>        // _open_osfhandle
#  include <fcntl.h>     // _O_TEXT
#endif

#include <string>
#include <vector>
#include <cstddef>     // size_t
#include <sstream>
#include <fstream>
#include <iostream>
#include <ext/stdio_filebuf.h>

#include <libcutl/fs/path.hxx>
#include <libcutl/fs/auto-remove.hxx>

#include <odb/version.hxx>
#include <odb/options.hxx>
#include <odb/profile.hxx>

#ifdef HAVE_CONFIG_H
#  include <odb/config.h>
#endif

using namespace std;
using cutl::fs::path;
using cutl::fs::invalid_path;
using cutl::fs::auto_remove;

typedef vector<string> strings;
typedef vector<path> paths;

//
// Path manipulation.
//

// Escape backslashes in the path.
//
static string
escape_path (string const&);

// Search the PATH environment variable for the file.
//
static path
path_search (path const&);

// Driver path with the directory part (search PATH).
//
static path
driver_path (path const& driver);

#ifndef ODB_STATIC_PLUGIN
static path
plugin_path (path const& driver, string const& gxx);
#endif

//
// Process manipulation.
//
struct process_info
{
#ifndef _WIN32
  pid_t id;
#else
  HANDLE id;
#endif

  int out_fd; // Write to this fd to send to the new process' stdin.
  int in_efd; // Read from this fd to receive from the new process' stderr.
  int in_ofd; // Read from this fd to receive from the new process' stdout.
};

struct process_failure {};

#ifdef _WIN32
// Deal with Windows command line length limit.
//
static auto_remove
fixup_cmd_line (vector<const char*>& args,
                size_t start,
                const char* name,
                string& arg);
#endif

// Start another process using the specified command line. Connect the
// newly created process' stdin to out_fd. Also if connect_* are true,
// connect the created process' stdout and stderr to in_*fd. Issue
// diagnostics and throw process_failure if anything goes wrong. The
// name argument is the name of the current process for diagnostics.
//
static process_info
start_process (char const* args[],
               char const* name,
               bool connect_stderr = false,
               bool connect_stdout = false);

// Wait for the process to terminate. Return true if the process terminated
// normally and with the zero exit status. Issue diagnostics and throw
// process_failure if anything goes wrong. The name argument is the name
// of the current process for diagnostics.
//
static bool
wait_process (process_info, char const* name);

//
//
static string
encode_plugin_flag (string const& k);

static string
encode_plugin_option (string const& k, string const& v);

// Extract header search paths from GCC's -v output. May throw the
// profile_failure, process_failure and invalid_path exceptions. Name
// is the program name (argv[0]) for diagnostics.
//
static paths
profile_paths (strings const& args, char const* name);

static char const* const db_macro[] =
{
  "-DODB_DATABASE_COMMON",
  "-DODB_DATABASE_MSSQL",
  "-DODB_DATABASE_MYSQL",
  "-DODB_DATABASE_ORACLE",
  "-DODB_DATABASE_PGSQL",
  "-DODB_DATABASE_SQLITE"
};

int
main (int argc, char* argv[])
{
  ostream& e (cerr);

  try
  {
    strings args, plugin_args;
    bool v (false);

    // The first argument points to the program name, which is
    // g++ by default.
    //
#ifdef ODB_GXX_NAME
    path gxx (ODB_GXX_NAME);

    if (gxx.empty ())
    {
      e << argv[0] << ": error: embedded g++ compile name is empty" << endl;
      return 1;
    }

    // If the g++ name is a relative path (starts with '.'), then use
    // our own path as base.
    //
    if (gxx.string ()[0] == '.')
    {
      path dp (driver_path (path (argv[0])));
      path d (dp.directory ());

      if (!d.empty ())
        gxx = d / gxx;
    }

    args.push_back (gxx.string ());

    // Also modify LD_LIBRARY_PATH to include the lib path.
    //
    // @@ Hm, I wonder why we are doing this? (One some platforms, like
    //    Fedora, the directory could be called something else, like lib64).
    //
#ifndef _WIN32
    {
#ifdef __APPLE__
      char const name[] = "DYLD_LIBRARY_PATH";
#else
      char const name[] = "LD_LIBRARY_PATH";
#endif

      string ld_paths;

      if (char const* s = getenv (name))
        ld_paths = s;

      path d (gxx.directory ());

      if (!d.empty ())
      {
        d.complete ();
        d /= path ("..") / path ("lib");

        if (ld_paths.empty ())
          ld_paths = d.string ();
        else
          ld_paths = d.string () + path::traits::path_separator + ld_paths;

        if (setenv (name, ld_paths.c_str (), 1) != 0)
        {
          e << argv[0] << ": error: unable to update environment" << endl;
          return 1;
        }
      }
    }
#endif // _WIN32

#else
    args.push_back ("g++");
#endif // ODB_GXX_NAME

    // Default options.
    //
    args.push_back ("-x");
    args.push_back ("c++");
    args.push_back (""); // Reserve space for -std=c++XX.
    args.push_back ("-S");
    args.push_back ("-Wunknown-pragmas");
    args.push_back ("-Wno-deprecated");
    args.push_back (""); // Reserve space for -fplugin=path.

    // Parse the default options file if we have one.
    //
    strings def_inc_dirs;
    strings def_defines;
#ifdef ODB_DEFAULT_OPTIONS_FILE
    {
      path file (ODB_DEFAULT_OPTIONS_FILE);

      // If the path is relative, then use the driver's path as a base. If
      // the file is not found in that directory, then also try outer
      // directory (so that we can find /etc if driver is in /usr/bin).
      //
      if (file.relative ())
      {
        bool found (false);
        path dd (driver_path (path (argv[0])).directory ());

        for (path d (dd);; d = d.directory ())
        {
          path f (d / file);
          // Check that the file exist without checking for permissions, etc.
          //
          struct stat s;
          if (stat (f.string ().c_str (), &s) == 0 && S_ISREG (s.st_mode))
          {
            file = f;
            found = true;
            break;
          }

          if (d.root ())
            break;
        }

        if (!found)
          file = dd / file; // For diagnostics.
      }

      cli::argv_file_scanner s (file.string ());

      bool first_x (true);

      while (s.more ())
      {
        string a (s.next ());
        size_t n (a.size ());

        // -x
        //
        if (a == "-x")
        {
          if (!s.more () || (a = s.next ()).empty ())
          {
            e << file << ": error: expected argument for the " << a
              << " option" << endl;
            return 1;
          }

          if (first_x)
          {
            first_x = false;

            // If it doesn't start with '-', then it must be the g++
            // executable name. Update the first argument with it.
            //
            if (a[0] != '-')
              args[0] = a;
            else
              args.push_back (a);
          }
          else
            args.push_back (a);
        }
        // -I
        //
        else if (n > 1 && a[0] == '-' && a[1] == 'I')
        {
          def_inc_dirs.push_back (a);

          if (n == 2) // -I /path
          {
            if (!s.more () || (a = s.next ()).empty ())
            {
              e << file << ": error: expected argument for the -I option"
                << endl;
              return 1;
            }

            def_inc_dirs.push_back (a);
          }
        }
        // -isystem, -iquote, -idirafter, and -framework (Mac OS X)
        //
        else if (a == "-isystem"   ||
                 a == "-iquote"    ||
                 a == "-idirafter" ||
                 a == "-framework")
        {
          def_inc_dirs.push_back (a);

          if (!s.more () || (a = s.next ()).empty ())
          {
            e << file << ": error: expected argument for the " << a
              << " option" << endl;
            return 1;
          }

          def_inc_dirs.push_back (a);
        }
        // -D
        //
        else if (n > 1 && a[0] == '-' && a[1] == 'D')
        {
          def_defines.push_back (a);

          if (n == 2) // -D macro
          {
            if (!s.more () || (a = s.next ()).empty ())
            {
              e << file << ": error: expected argument for the -D option"
                << endl;
              return 1;
            }

            def_defines.push_back (a);
          }
        }
        // -U
        //
        else if (n > 1 && a[0] == '-' && a[1] == 'U')
        {
          def_defines.push_back (a);

          if (n == 2) // -U macro
          {
            if (!s.more () || (a = s.next ()).empty ())
            {
              e << file << ": error: expected argument for the -U option"
                << endl;
              return 1;
            }

            def_defines.push_back (a);
          }
        }
        else
          plugin_args.push_back (a);
      }
    }
#endif

    // Add the default preprocessor defines (-D/-U) before the user-supplied
    // ones.
    //
    args.insert (args.end (), def_defines.begin (), def_defines.end ());

    // Parse driver options.
    //
    // We scan expanding --options-file in order to allow specifying ad hoc
    // options (-I, etc) in options files.
    //
    bool first_x (true);

    for (cli::argv_file_scanner scan (argc, argv, "--options-file");
         scan.more (); )
    {
      string a (scan.next ());
      size_t n (a.size ());

      // -v
      //
      if (a == "-v")
      {
        v = true;
        args.push_back (a);
      }
      // -x
      //
      else if (a == "-x")
      {
        const char* v;
        if (!scan.more () || (v = scan.next ())[0] == '\0')
        {
          e << argv[0] << ": error: expected argument for the -x option"
            << endl;
          return 1;
        }

        if (first_x)
        {
          first_x = false;

          // If it doesn't start with '-', then it must be the g++
          // executable name. Update the first argument with it.
          //
          if (v[0] != '-')
            args[0] = v;
          else
            args.push_back (v);
        }
        else
          args.push_back (v);
      }
      // -I
      //
      else if (n > 1 && a[0] == '-' && a[1] == 'I')
      {
        args.push_back (a);

        if (n == 2) // -I /path
        {
          const char* v;
          if (!scan.more () || (v = scan.next ())[0] == '\0')
          {
            e << argv[0] << ": error: expected argument for the -I option"
              << endl;
            return 1;
          }

          args.push_back (v);
        }
      }
      // -isystem, -iquote, -idirafter, and -framework (Mac OS X)
      //
      else if (a == "-isystem"   ||
               a == "-iquote"    ||
               a == "-idirafter" ||
               a == "-framework")
      {
        args.push_back (a);

        const char* v;
        if (!scan.more () || (v = scan.next ())[0] == '\0')
        {
          e << argv[0] << ": error: expected argument for the " << a
            << " option" << endl;
          return 1;
        }

        args.push_back (v);
      }
      // -D
      //
      else if (n > 1 && a[0] == '-' && a[1] == 'D')
      {
        args.push_back (a);

        if (n == 2) // -D macro
        {
          const char* v;
          if (!scan.more () || (v = scan.next ())[0] == '\0')
          {
            e << argv[0] << ": error: expected argument for the -D option"
              << endl;
            return 1;
          }

          args.push_back (v);
        }
      }
      // -U
      //
      else if (n > 1 && a[0] == '-' && a[1] == 'U')
      {
        args.push_back (a);

        if (n == 2) // -U macro
        {
          const char* v;
          if (!scan.more () || (v = scan.next ())[0] == '\0')
          {
            e << argv[0] << ": error: expected argument for the -U option"
              << endl;
            return 1;
          }

          args.push_back (v);
        }
      }
      // Store everything else in a list so that we can parse it with the
      // cli parser. This is the only reliable way to find out where the
      // options end.
      //
      else
        plugin_args.push_back (a);
    }

    // Add the default include directories (-I) after the user-supplied
    // ones.
    //
    args.insert (args.end (), def_inc_dirs.begin (), def_inc_dirs.end ());

    // Find the plugin.
    //
    {
#ifndef ODB_STATIC_PLUGIN
      path plugin (plugin_path (path (argv[0]), args[0]));
#else
      // Use a dummy name if the plugin is linked into the compiler.
      //
      path plugin ("odb");
#endif

      if (plugin.empty ())
        return 1; // Diagnostics has already been issued.

#ifdef _WIN32
      // Here is the problem: since the plugin is loaded by GCC (cc1plus.exe
      // to be precise), the DLL assembly magic we have for executables won't
      // help here.
      //
      // To allow executing the ODB compiler in-place we add the odb.exe.dlls/
      // directory to PATH. It is a bit of hack but then DLL assemblies for
      // DLLs is a whole new level of insanity that we are unlikely to ever
      // touch.
      //
      // And it turns out we have the same problem in the installed case: if
      // the installation directory is not in PATH, then GCC won't find the
      // DLLs the plugin needs. So we handle both cases here.
      //
      {
        path d (plugin.directory ());
        d.complete ();
        d.normalize ();
        d /= path ("odb.exe.dlls");

        struct stat st;
        if (stat (d.string ().c_str (), &st) != 0 || !S_ISDIR (st.st_mode))
          d = d.directory ();

        string v ("PATH=" + d.string ());

        if (char const* p = getenv ("PATH"))
        {
          v += ';';
          v += p;
        }

        _putenv (v.c_str ());
      }
#endif

      args[7] = "-fplugin=" + plugin.string ();
    }

    // Parse plugin options. We have to do it twice to get the target
    // database which is needed while loading profiles.
    //
    vector<char*> av;
    av.push_back (argv[0]);

    for (strings::iterator i (plugin_args.begin ()), end (plugin_args.end ());
         i != end; ++i)
    {
      av.push_back (const_cast<char*> (i->c_str ()));
    }

    int ac (static_cast<int> (av.size ()));

    cli::argv_file_scanner::option_info oi[3];
    oi[0].option = "--options-file"; // Keep in case profile uses it.
    oi[0].search_func = 0;
    oi[1].option = "-p";
    oi[2].option = "--profile";

    vector<database> dbs;
    bool show_sloc;
    size_t sloc_limit;
    {
      oi[1].search_func = &profile_search_ignore;
      oi[2].search_func = &profile_search_ignore;

      cli::argv_file_scanner scan (ac, &av[0], oi, 3);
      options ops (scan);

      // Handle --build2-metadata (see also buildfile).
      //
      if (ops.build2_metadata_specified ())
      {
        ostream& o (cout);

        // Note that the export.metadata variable should be the first non-
        // blank/comment line.
        //
        o << "# build2 buildfile odb" << endl
          << "export.metadata = 1 odb" << endl
          << "odb.name = [string] odb" << endl
          << "odb.version = [string] '" << ODB_COMPILER_VERSION_FULL << '\'' << endl
          << "odb.checksum = [string] '" << ODB_COMPILER_VERSION_FULL << '\'' << endl
          << "odb.environment = [strings] CPATH CPLUS_INCLUDE_PATH GCC_EXEC_PREFIX COMPILER_PATH" << endl;

        return 0;
      }

      // Handle --version.
      //
      if (ops.version ())
      {
        ostream& o (cout);

        o << "ODB object-relational mapping (ORM) compiler for C++ "
          ODB_COMPILER_VERSION_STR << endl
          << "Copyright (c) " << ODB_COPYRIGHT << "." << endl
          << "This is free software; see the source for copying conditions. "
          << "There is NO\nwarranty; not even for MERCHANTABILITY or FITNESS "
          << "FOR A PARTICULAR PURPOSE." << endl;

        return 0;
      }

      // Handle --help.
      //
      if (ops.help ())
      {
        ostream& o (cout);

        o << "Usage: " << argv[0] << " [options] file [file ...]" << endl
          << "Options:" << endl;

        options::print_usage (cout);
        return 0;
      }

      // Check that required options were specifed.
      //
      dbs = ops.database ();

      if (dbs.empty ())
      {
        e << argv[0] << ": error: no database specified with the --database "
          << "option" << endl;
        return 1;
      }

      if (dbs.size () > 1 && !ops.multi_database_specified ())
      {
        e << argv[0] << ": error: --multi-database option required when " <<
          "multiple databases are specified"<< endl;
        return 1;
      }

      show_sloc = ops.show_sloc ();
      sloc_limit = ops.sloc_limit_specified () ? ops.sloc_limit () : 0;

      // Translate some ODB options to GCC options.
      //
      switch (ops.std ())
      {
      case cxx_version::cxx98:
        {
          args[3] = "-std=c++98";
          break;
        }
      case cxx_version::cxx11:
        {
          args[3] = "-std=c++0x"; // c++11 was only added in GCC 4.7.0.
          break;
        }
      case cxx_version::cxx14:
        {
          args[3] = "-std=c++1y";
          break;
        }
      case cxx_version::cxx17:
        {
          args[3] = "-std=c++1z";
          break;
        }
      case cxx_version::cxx20:
        {
          args[3] = "-std=c++2a";
          break;
        }
      }
    }

    // Obtain profile (-I) search paths.
    //
    paths prof_paths (profile_paths (args, argv[0]));

    if (v)
    {
      e << "Profile search paths:" << endl;

      for (paths::const_iterator i (prof_paths.begin ());
           i != prof_paths.end (); ++i)
        e << " " << *i << endl;
    }

    // Pass profile search paths (svc-path option).
    //
    for (paths::const_iterator i (prof_paths.begin ());
         i != prof_paths.end (); ++i)
    {
      args.push_back (encode_plugin_option ("svc-path", i->string ()));
    }

    // Add common ODB macros.
    //
    args.push_back ("-DODB_COMPILER");

    {
      ostringstream ostr;
      ostr << ODB_COMPILER_VERSION_OLD;
      args.push_back ("-DODB_COMPILER_VERSION=" + ostr.str ());
    }

    // Compile for each database.
    //
    size_t sloc_total (0);

    for (vector<database>::iterator i (dbs.begin ()); i != dbs.end (); ++i)
    {
      database db (*i);
      strings db_args (args);

      // Add database-specific ODB macro.
      //
      db_args.push_back (db_macro[db]);

      // Second parse.
      //
      profile_data pd (prof_paths, db, argv[0]);
      oi[1].search_func = &profile_search;
      oi[2].search_func = &profile_search;
      oi[1].arg = &pd;
      oi[2].arg = &pd;

      cli::argv_file_scanner scan (ac, &av[0], oi, 3);
      options ops (scan);

      size_t end (scan.end () - 1); // We have one less in plugin_args.

      if (end == plugin_args.size ())
      {
        e << argv[0] << ": error: input file expected" << endl;
        return 1;
      }

      // Encode plugin options.
      //
      // Add the database we are compiling for first. More databases
      // could be specified in options files but they will be ignored
      // by the plugin (it only cares about the first).
      //
      db_args.push_back (encode_plugin_option ("database", db.string ()));

      cli::options const& desc (options::description ());
      for (size_t i (0); i < end; ++i)
      {
        string k, a (plugin_args[i]);

        // Ignore certain options.
        //
        if (a == "--")
        {
          // Ignore the option seperator since GCC doesn't understand it.
          //
          continue;
        }
        else if (a == "-d" || a == "--database")
        {
          // Ignore all other databases.
          //
          i++; // Skip the value.
          continue;
        }

        cli::options::const_iterator it (desc.find (a));

        if (it == desc.end ())
        {
          e << argv[0] << ": ice: unexpected option '" << a << "'" << endl;
          return 1;
        }

        if (a.size () > 2 && a[0] == '-' && a[1] == '-')
          k = string (a, 2); // long format
        else
          k = string (a, 1); // short format

        if (it->flag ())
          db_args.push_back (encode_plugin_flag (k));
        else
        {
          // If there are more arguments then we may have a value.
          //
          if (i + 1 == end)
          {
            e << argv[0] << ": ice: expected argument for '" << a << "'"
              << endl;
            return 1;
          }

          db_args.push_back (encode_plugin_option (k, plugin_args[++i]));
        }
      }

      // Reserve space for and remember the position of the svc-file
      // option.
      //
      size_t svc_file_pos (db_args.size ());
      db_args.push_back ("");

      // If compiling multiple input files at once, pass them also with
      // the --svc-file option.
      //
      bool at_once (ops.at_once () && plugin_args.size () - end > 1);
      if (at_once)
      {
        if (ops.input_name ().empty ())
        {
          e << "error: --input-name required when compiling multiple " <<
            "input files at once (--at-once)" << endl;
          return 1;
        }

        for (size_t i (end); i < plugin_args.size (); ++i)
          db_args.push_back (
            encode_plugin_option ("svc-file", plugin_args[i]));
      }

      // Create an execvp-compatible argument array.
      //
      typedef vector<char const*> cstrings;
      cstrings exec_args;

      for (strings::const_iterator i (db_args.begin ()), end (db_args.end ());
           i != end; ++i)
      {
        exec_args.push_back (i->c_str ());
      }

      exec_args.push_back ("-"); // Compile stdin.
      exec_args.push_back (0);

      // Iterate over the input files and compile each of them.
      //
      for (; end < plugin_args.size (); ++end)
      {
        string name (at_once ? ops.input_name () : plugin_args[end]);

        // Set the --svc-file option.
        //
        db_args[svc_file_pos] = encode_plugin_option ("svc-file", name);
        exec_args[svc_file_pos] = db_args[svc_file_pos].c_str ();

        //
        //
        ifstream ifs;

        if (!at_once)
        {
          ifs.open (name.c_str (), ios_base::in | ios_base::binary);

          if (!ifs.is_open ())
          {
            e << name << ": error: unable to open in read mode" << endl;
            return 1;
          }
        }

        if (v)
        {
          e << "Compiling " << name << endl;
          for (cstrings::const_iterator i (exec_args.begin ());
               i != exec_args.end (); ++i)
          {
            if (*i != 0)
              e << *i << (*(i + 1) != 0 ? ' ' : '\n');
          }
        }

        // Deal with Windows command line length limit.
        //
#ifdef _WIN32
        string ops_file_arg;
        auto_remove opt_file_rm (
          fixup_cmd_line (exec_args, 1, argv[0], ops_file_arg));
#endif

        process_info pi (start_process (&exec_args[0], argv[0], false, true));

        {
          __gnu_cxx::stdio_filebuf<char> fb (
            pi.out_fd, ios_base::out | ios_base::binary);
          ostream os (&fb);

          if (!at_once)
          {
            // See if we there is a UTF-8 BOM in the input file. If so,
            // then we need to write it before prologues.
            //
            if (ifs.peek () == 0xEF)
            {
              ifs.get ();
              if (ifs.get () != 0xBB || ifs.get () != 0xBF)
              {
                e << name << ": error: invalid UTF-8 BOM sequence" << endl;
                fb.close ();
                wait_process (pi, argv[0]);
                return 1;
              }

              os.put (0xEF);
              os.put (0xBB);
              os.put (0xBF);
            }
          }

          if (!ops.trace ())
          {
            // Add the standard prologue.
            //
            os << "#line 1 \"<standard-odb-prologue>\"" << endl;

            // Make sure ODB compiler and libodb versions are compatible.
            //
            // Note that the snapshot comparision is only necessary of the
            // version is a pre-release but having it always won't hurt (it
            // will be 0 for final versions).
            //
            // After some experience with requiring the exact version match we
            // found it just too tedious and went back to only comparing the
            // interface version (we could support both with an option; see
            // also similar code in generator.cxx).
            //
            os << "#include <odb/version.hxx>" << endl
               << endl
#if 1
               << "#if ODB_VERSION != " << ODB_VERSION << "UL" << endl
#else
               << "#if LIBODB_VERSION_FULL != " << ODB_COMPILER_VERSION << "ULL"
              " || LIBODB_SNAPSHOT != " << ODB_COMPILER_SNAPSHOT << "ULL" << endl
#endif
               << "#  error incompatible ODB compiler and runtime " <<
              "versions" << endl
               << "#endif" << endl
               << endl;

            // Include std::string. It is used as a default type for
            // the implicit discriminator member in polymorphism
            // support.
            //
            os << "#include <string>" << endl
               << endl;

            // Add ODB compiler metaprogramming tests.
            //
            os << "namespace odb" << endl
               << "{" << endl
               << "namespace compiler" << endl
               << "{" << endl;

            // operator< test, used in validator.
            //
            // Note that typeof() cannot be used in the function signature
            // directly so we have to go though lt_operator_type. This means
            // we get diagnostics from the compiler (followed by ours) but
            // it's doesn't look bad plus C++98 support is on its way out.
            //
            os << "template <typename T>" << endl
               << "const T&" << endl
               << "instance ();" << endl
               << endl;

            if (ops.std () == cxx_version::cxx98)
            {
              os << "template <typename T>" << endl
                 << "struct lt_operator_type" << endl
                 << "{" << endl
                 << "typedef __typeof__ (instance<T> () < instance<T> ()) R;" << endl
                 << "};" << endl
                 << endl
                 << "template <typename T>" << endl
                 << "typename lt_operator_type<T>::R" << endl
                 << "has_lt_operator ();" << endl;
            }
            else
            {
              os << "template <typename T>" << endl
                 << "decltype (instance<T> () < instance<T> ())" << endl
                 << "has_lt_operator ();" << endl;
            }

            os << "}" << endl
               << "}" << endl;
          }

          // Add custom prologue if any.
          //
          // NOTE: if you change the format, you also need to update code
          // in include.cxx
          //
          size_t pro_count (1);
          if (ops.odb_prologue ().count (db) != 0)
          {
            strings const& pro (ops.odb_prologue ()[db]);
            for (size_t i (0); i < pro.size (); ++i, ++pro_count)
            {
              os << "#line 1 \"<odb-prologue-" << pro_count << ">\"" << endl
                 << pro[i] << endl;
            }
          }

          if (ops.odb_prologue_file ().count (db) != 0)
          {
            strings const& prof (ops.odb_prologue_file ()[db]);
            for (size_t i (0); i < prof.size (); ++i, ++pro_count)
            {
              os << "#line 1 \"<odb-prologue-" << pro_count << ">\""
                 << endl;

              ifstream ifs (prof[i].c_str (), ios_base::in | ios_base::binary);

              if (!ifs.is_open ())
              {
                e << prof[i] << ": error: unable to open in read mode" << endl;
                fb.close ();
                wait_process (pi, argv[0]);
                return 1;
              }

              if (!(os << ifs.rdbuf ()))
              {
                e << prof[i] << ": error: io failure" << endl;
                fb.close ();
                wait_process (pi, argv[0]);
                return 1;
              }

              os << endl;
            }
          }

          if (at_once)
          {
            // Include all the input files (no need to escape).
            //
            os << "#line 1 \"<command-line>\"" << endl;

            bool b (ops.include_with_brackets ());
            char op (b ? '<' : '"'), cl (b ? '>' : '"');

            for (; end < plugin_args.size (); ++end)
              os << "#include " << op << plugin_args[end] << cl << endl;
          }
          else
          {
            // Write the synthesized translation unit to stdout.
            //
            os << "#line 1 \"" << escape_path (name) << "\"" << endl;

            if (!(os << ifs.rdbuf ()))
            {
              e << name << ": error: io failure" << endl;
              fb.close ();
              wait_process (pi, argv[0]);
              return 1;
            }

            // Add a new line in case the input file doesn't end with one.
            //
            os << endl;
          }

          // Add custom epilogue if any.
          //
          // NOTE: if you change the format, you also need to update code
          // in include.cxx
          //
          size_t epi_count (1);
          if (ops.odb_epilogue ().count (db) != 0)
          {
            strings const& epi (ops.odb_epilogue ()[db]);
            for (size_t i (0); i < epi.size (); ++i, ++epi_count)
            {
              os << "#line 1 \"<odb-epilogue-" << epi_count << ">\"" << endl
                 << epi[i] << endl;
            }
          }

          if (ops.odb_epilogue_file ().count (db) != 0)
          {
            strings const& epif (ops.odb_epilogue_file ()[db]);
            for (size_t i (0); i < epif.size (); ++i, ++epi_count)
            {
              os << "#line 1 \"<odb-epilogue-" << epi_count << ">\""
                 << endl;

              ifstream ifs (epif[i].c_str (), ios_base::in | ios_base::binary);

              if (!ifs.is_open ())
              {
                e << epif[i] << ": error: unable to open in read mode" << endl;
                fb.close ();
                wait_process (pi, argv[0]);
                return 1;
              }

              if (!(os << ifs.rdbuf ()))
              {
                e << epif[i] << ": error: io failure" << endl;
                fb.close ();
                wait_process (pi, argv[0]);
                return 1;
              }

              os << endl;
            }
          }

          if (!ops.trace ())
          {
            // Add the standard epilogue at the end so that we see all
            // the declarations.
            //
            os << "#line 1 \"<standard-odb-epilogue>\"" << endl;

            // Includes for standard smart pointers. The Boost TR1 header
            // may or may not delegate to the GCC implementation. In either
            // case, the necessary declarations will be provided so we don't
            // need to do anything.
            //
            os << "#include <memory>" << endl;

            // Standard wrapper traits.
            //
            os << "#include <odb/wrapper-traits.hxx>" << endl;

            // Standard pointer traits.
            //
            os << "#include <odb/pointer-traits.hxx>" << endl;

            // Standard container traits.
            //
            os << "#include <odb/container-traits.hxx>" << endl;

            // TR1 wrapper/pointer traits.
            //
            // @@ TMP: drop after 2.5.0.
#if 0
            if (ops.std () == cxx_version::cxx98)
              os << endl
                 << "#ifndef BOOST_TR1_MEMORY_HPP_INCLUDED" << endl
                 << "#  include <tr1/memory>" << endl
                 << "#endif" << endl
                 << "#include <odb/tr1/wrapper-traits.hxx>" << endl
                 << "#include <odb/tr1/pointer-traits.hxx>" << endl;
#endif
          }
        }

        // Filter the output stream looking for communication from the
        // plugin.
        //
        {
          __gnu_cxx::stdio_filebuf<char> fb (pi.in_ofd, ios_base::in);
          istream is (&fb);

          for (bool first (true); !is.eof (); )
          {
            string line;
            getline (is, line);

            if (is.fail () && !is.eof ())
            {
              e << argv[0] << ": error: io failure while parsing output"
                << endl;
              wait_process (pi, argv[0]);
              return 1;
            }

            if (line.compare (0, 9, "odb:sloc:") == 0)
            {
              if (show_sloc || sloc_limit != 0)
              {
                size_t n;
                istringstream is (string (line, 9, string::npos));

                if (!(is >> n && is.eof ()))
                {
                  e << argv[0] << ": error: invalid odb:sloc value" << endl;
                  wait_process (pi, argv[0]);
                  return 1;
                }

                sloc_total += n;
              }

              continue;
            }

            if (first)
              first = false;
            else
              cout << endl;

            cout << line;
          }
        }

        if (!wait_process (pi, argv[0]))
          return 1;
      } // End input file loop.
    } // End database loop.

    // Handle SLOC.
    //
    if (show_sloc)
      e << "total: " << sloc_total << endl;

    if (sloc_limit != 0 && sloc_limit < sloc_total)
    {
      e << argv[0] << ": error: SLOC limit of " << sloc_limit << " lines " <<
        "has been exceeded" << endl;

      if (!show_sloc)
        e << argv[0] << ": info: use the --show-sloc option to see the "
          << "current total" << endl;

      return 1;
    }
  }
  catch (profile_failure const&)
  {
    // Diagnostics has already been issued.
    //
    return 1;
  }
  catch (process_failure const&)
  {
    // Diagnostics has already been issued.
    //
    return 1;
  }
  catch (invalid_path const& ex)
  {
    e << argv[0] << ": error: invalid path '" << ex.path () << "'" << endl;
    return 1;
  }
  catch (cli::exception const& ex)
  {
    e << ex << endl;
    return 1;
  }
}

static inline string
encode_plugin_flag (string const& k)
{
  return "-fplugin-arg-odb-" + k;
}

static string
encode_plugin_option (string const& k, string const& cv)
{
  string o ("-fplugin-arg-odb-");
  o += k;
  o += '=';

  if (!cv.empty ())
  {
    // A value cannot contain '='. Encode it as the backspace
    // character.
    //
    string v (cv);
    for (size_t i (0); i < v.size (); ++i)
      if (v[i] == '=')
        v[i] = '\b';

    o += v;
  }

  return o;
}

static paths
profile_paths (strings const& sargs, char const* name)
{
  // Copy some of the arguments from the passed list. We also need
  // the g++ executable.
  //
  strings args;

  args.push_back (sargs[0]);
  args.push_back ("-v");
  args.push_back ("-x");
  args.push_back ("c++");
  args.push_back ("-E");
  args.push_back ("-P");

  for (strings::const_iterator i (++sargs.begin ()), end (sargs.end ());
       i != end; ++i)
  {
    string const& a (*i);

    // -I
    //
    if (a.size () > 1 && a[0] == '-' && a[1] == 'I')
    {
      args.push_back (a);

      if (a.size () == 2) // -I /path
      {
        args.push_back (*(++i));
      }
    }
    // -framework
    //
    else if (a == "-isystem"   ||
             a == "-iquote"    ||
             a == "-idirafter" ||
             a == "-isysroot"  ||
             a == "-framework")
    {
      args.push_back (a);

      if (++i == end)
      {
        cerr << name << ": error: expected argument for the " << a
             << " option" << endl;
        throw profile_failure ();
      }

      args.push_back (*i);
    }
    // --sysroot
    //
    else if (a.compare (0, 10, "--sysroot=") == 0)
      args.push_back (a);
    // -std
    //
    else if (a.compare (0, 5, "-std=") == 0)
      args.push_back (a);
  }

  // Create an execvp-compatible argument array.
  //
  vector<char const*> exec_args;

  for (strings::const_iterator i (args.begin ()), end (args.end ());
       i != end; ++i)
  {
    exec_args.push_back (i->c_str ());
  }

  exec_args.push_back ("-"); // Compile stdin.
  exec_args.push_back (0);

#ifdef _WIN32
  string ops_file_arg;
  auto_remove opt_file_rm (fixup_cmd_line (exec_args, 1, name, ops_file_arg));
#endif

  process_info pi (start_process (&exec_args[0], name, true));
  close (pi.out_fd); // Preprocess empty file.

  // Read the output into a temporary string stream. We don't parse
  // it on the fly because we don't know whether it is the data or
  // diagnostics until after the process is terminated and we get
  // the exit code. We also cannot first wait for the exist code
  // and then read the output because the process might get blocked.
  //
  stringstream ss;
  {
    __gnu_cxx::stdio_filebuf<char> fb (pi.in_efd, ios_base::in);
    istream is (&fb);

    for (bool first (true); !is.eof (); )
    {
      string line;
      getline (is, line);

      if (is.fail () && !is.eof ())
      {
        cerr << name << ": error: "
             << "io failure while parsing profile paths" << endl;

        wait_process (pi, name);
        throw profile_failure ();
      }

      if (first)
        first = false;
      else
        ss << endl;

      ss << line;
    }
  }

  if (!wait_process (pi, name))
  {
    // Things didn't go well and ss should contain the diagnostics.
    // In case it is empty, issue our own.
    //
    if (!ss.str ().empty ())
      cerr << ss.rdbuf ();
    else
      cerr << name << ": error: unable to extract profile paths" << endl;

    throw profile_failure ();
  }

  // Parse the cached output.
  //
  paths r;
  {
    enum
    {
      read_prefix,
      read_path,
      read_suffix
    } state = read_prefix;

    while (!ss.eof () && state != read_suffix)
    {
      string line;
      getline (ss, line);

      if (ss.fail () && !ss.eof ())
      {
        cerr << name << ": error: "
             << "io failure while parsing profile paths" << endl;
        throw profile_failure ();
      }

      switch (state)
      {
      case read_prefix:
        {
          // The English string that we are looking for is "#include <...>
          // search starts here:" but it can be translated. However, all
          // the translations seems to have the "#include" and "<...>"
          // parts, so we can search for those.
          //
          if (line.find ("#include") != string::npos &&
              line.find ("<...>") != string::npos)
            state = read_path;
          break;
        }
      case read_path:
        {
          // The end of the list is terminated with the "End of search
          // list." line, which, again, can be translated. Here we don't
          // have any invariable parts that we can use. Instead, we will
          // rely on the fact that all the paths are space-indented.
          //
          if (!line.empty () && line[0] != ' ')
            state = read_suffix;
          else
            // Paths are indented with a space.
            //
            r.push_back (path (string (line, 1)));

          break;
        }
      case read_suffix:
        {
          // We shouldn't get here.
          break;
        }
      }
    }

    if (state != read_suffix)
    {
      cerr << name << ": error: unable to parse profile paths" << endl;
      throw profile_failure ();
    }
  }

  return r;
}

//
// Path manipulation.
//

static string
escape_path (string const& p)
{
  string r;

  for (size_t i (0); i < p.size (); ++i)
  {
    if (p[i] == '\\')
      r += "\\\\";
    else
      r += p[i];
  }

  return r;
}

static path
path_search (path const& f)
{
  typedef path::traits traits;

  // If there is a directory component in the file, then the PATH
  // search does not apply.
  //
  if (!f.directory ().empty ())
    return f;

  string paths;

  // If there is no PATH in environment then the default search
  // path is the current directory.
  //
  if (char const* s = getenv ("PATH"))
    paths = s;
  else
    paths = traits::path_separator;

  // On Windows also check the current directory.
  //
#ifdef _WIN32
  paths += traits::path_separator;
#endif

  struct stat info;

  for (size_t b (0), e (paths.find (traits::path_separator));
       b != string::npos;)
  {
    path p (string (paths, b, e != string::npos ? e - b : e));

    // Empty path (i.e., a double colon or a colon at the beginning
    // or end of PATH) means search in the current dirrectory.
    //
    if (p.empty ())
      p = path (".");

    path dp (p / f);

    // Just check that the file exist without checking for permissions, etc.
    //
    if (stat (dp.string ().c_str (), &info) == 0 && S_ISREG (info.st_mode))
      return dp;

    // On Windows also try the path with the .exe extension.
    //
#ifdef _WIN32
    dp += ".exe";

    if (stat (dp.string ().c_str (), &info) == 0 && S_ISREG (info.st_mode))
      return dp;
#endif

    if (e == string::npos)
      b = e;
    else
    {
      b = e + 1;
      e = paths.find (traits::path_separator, b);
    }
  }

  return path ();
}

static path
driver_path (path const& drv)
{
  return drv.directory ().empty () ? path_search (drv) : drv;
}

#ifndef ODB_STATIC_PLUGIN
static path
plugin_path (path const& drv,
#ifdef ODB_GCC_PLUGIN_DIR
             string const& gxx)
#else
             string const&)
#endif
{
#ifdef _WIN32
  char const plugin_ext[] = ".dll";

// While GCC 8 switched to using .dylib as the plugin extension, there is a
// bug (86358) in the extension stripping code. So for now we use the .so
// extension everywhere (see also buildfile if changing this).
//
//#elif defined(__APPLE__)
//  char const plugin_ext[] = ".dylib";
#else
  char const plugin_ext[] = ".so";
#endif

  // Figure out the plugin base name which is just the driver name (but
  // without the .exe extension on Windows).
  //
#ifdef _WIN32
  string b (drv.leaf ().base ().string ());
#else
  string b (drv.leaf ().string ());
#endif

  path dp (driver_path (drv));

  if (dp.empty ())
  {
    cerr << drv << ": error: unable to resolve ODB compiler driver path" << endl;
    return path ();
  }

  dp = dp.directory ();
  struct stat info;

  path pp;

#ifdef ODB_GCC_PLUGIN_DIR
  // Plugin should be installed into the GCC default plugin directory.
  // Ideally, in this situation, we would simply pass the plugin name and
  // let GCC append the correct directory. Unfortunately, this mechanism
  // was only added in GCC 4.6 so in order to support 4.5 we will have to
  // emulate it ourselves.
  //
  //@@ TMP: drop this after 2.5.0 since we no longer support GCC < 5.
  //
  {
#if 1
    // First get the default GCC plugin directory.
    //
    path d;
    vector<char const*> exec_args;
    exec_args.push_back (gxx.c_str ());
    exec_args.push_back ("-print-file-name=plugin");
    exec_args.push_back (0);

    process_info pi (
      start_process (
        &exec_args[0], drv.string ().c_str (), false, true));
    close (pi.out_fd);

    // Read the path from stdout.
    //
    {
      __gnu_cxx::stdio_filebuf<char> fb (pi.in_ofd, ios_base::in);
      istream is (&fb);
      string line;
      getline (is, line);
      d = path (line);
    }

    if (!wait_process (pi, drv.string ().c_str ()))
      return path (); // Assume GCC issued some diagnostics.

    if (d.string () == "plugin")
    {
      cerr << drv << ": error: unable to obtain GCC plugin directory" << endl;
      return path ();
    }

    // See if the plugin is there.
    //
    pp = d / path (b + plugin_ext);
    if (stat (pp.string ().c_str (), &info) != 0)
    {
      cerr << drv << ": error: no ODB plugin in GCC plugin directory '" <<
        d << "'" << endl;
      return path ();
    }

    return pp;
#else
    return path (b);
#endif
  }
#elif defined (ODB_PLUGIN_PATH)
  // If we were given a plugin path, use that.
  //
  {
    string rp (ODB_PLUGIN_PATH);
    if (!rp.empty ())
      dp /= path (rp);

    pp = dp / path (b + plugin_ext);

    if (stat (pp.string ().c_str (), &info) != 0)
    {
      cerr << drv << ": error: no ODB plugin in '" << dp << "'" << endl;
      return path ();
    }

    return pp;
  }
#endif

  // Try in the current directory.
  //
  pp = dp / path (b + plugin_ext);
  if (stat (pp.string ().c_str (), &info) != 0)
  {
    cerr << drv << ": error: unable to locate ODB plugin" << endl;
    return path ();
  }

  return pp;
}
#endif

//
// Process manipulation.
//

#ifndef _WIN32

static process_info
start_process (char const* args[], char const* name, bool err, bool out)
{
  int out_fd[2];
  int in_efd[2];
  int in_ofd[2];

  if (pipe (out_fd) == -1 ||
      (err && pipe (in_efd) == -1) ||
      (out && pipe (in_ofd) == -1))
  {
    char const* err (strerror (errno));
    cerr << name << ": error: " <<  err << endl;
    throw process_failure ();
  }

  pid_t pid (fork ());

  if (pid == -1)
  {
    char const* err (strerror (errno));
    cerr << name << ": error: " <<  err << endl;
    throw process_failure ();
  }

  if (pid == 0)
  {
    // Child. Close the write end of the pipe and duplicate the read end
    // to stdin. Then close the original read end descriptors.
    //
    if (close (out_fd[1]) == -1 ||
        dup2 (out_fd[0], STDIN_FILENO) == -1 ||
        close (out_fd[0]) == -1)
    {
      char const* err (strerror (errno));
      cerr << name << ": error: " <<  err << endl;
      throw process_failure ();
    }

    // Do the same for the stderr if requested.
    //
    if (err)
    {
      if (close (in_efd[0]) == -1 ||
          dup2 (in_efd[1], STDERR_FILENO) == -1 ||
          close (in_efd[1]) == -1)
      {
        char const* err (strerror (errno));
        cerr << name << ": error: " <<  err << endl;
        throw process_failure ();
      }
    }

    // Do the same for the stdout if requested.
    //
    if (out)
    {
      if (close (in_ofd[0]) == -1 ||
          dup2 (in_ofd[1], STDOUT_FILENO) == -1 ||
          close (in_ofd[1]) == -1)
      {
        char const* err (strerror (errno));
        cerr << name << ": error: " <<  err << endl;
        throw process_failure ();
      }
    }

    if (execvp (args[0], const_cast<char**> (&args[0])) == -1)
    {
      char const* err (strerror (errno));
      cerr << args[0] << ": error: " << err << endl;
      throw process_failure ();
    }
  }
  else
  {
    // Parent. Close the other ends of the pipes.
    //
    if (close (out_fd[0]) == -1 ||
        (err && close (in_efd[1]) == -1) ||
        (out && close (in_ofd[1]) == -1))
    {
      char const* err (strerror (errno));
      cerr << name << ": error: " <<  err << endl;
      throw process_failure ();
    }
  }

  process_info r;
  r.id = pid;
  r.out_fd = out_fd[1];
  r.in_efd = err ? in_efd[0] : 0;
  r.in_ofd = out ? in_ofd[0] : 0;
  return r;
}

static bool
wait_process (process_info pi, char const* name)
{
  int status;

  if (waitpid (pi.id, &status, 0) == -1)
  {
    char const* err (strerror (errno));
    cerr << name << ": error: " <<  err << endl;
    throw process_failure ();
  }

  return WIFEXITED (status) && WEXITSTATUS (status) == 0;
}

#else // _WIN32

static void
print_error (char const* name)
{
  LPTSTR msg;
  DWORD e (GetLastError());

  if (!FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        0,
        e,
        MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &msg,
        0,
        0))
  {
    cerr << name << ": error: unknown error code " << e << endl;
    return;
  }

  cerr << name << ": error: " << msg << endl;
  LocalFree (msg);
}

// On Windows we need to protect command line arguments with spaces using
// quotes. Since there could be actual quotes in the value, we need to escape
// them.
//
static void
append_quoted (string& cmd_line, const char* ca)
{
  string a (ca);
  bool quote (a.find (' ') != string::npos);

  if (quote)
    cmd_line += '"';

  for (size_t i (0); i < a.size (); ++i)
  {
    if (a[i] == '"')
      cmd_line += "\\\"";
    else
      cmd_line += a[i];
  }

  if (quote)
    cmd_line += '"';
}

// Deal with Windows command line length limit.
//
// The best approach seems to be passing the command line in an "options file"
// ("response file" in Microsoft's terminology).
//
static auto_remove
fixup_cmd_line (vector<const char*>& args,
                size_t start,
                const char* name,
                string& arg)
{
  // Calculate the would-be command line length similar to how start_process()
  // implementation does it.
  //
  size_t n (0);
  string s;
  for (const char* a: args)
  {
    if (a != nullptr)
    {
      if (n != 0)
        n++; // For the space separator.

      s.clear ();
      append_quoted (s, a);
      n += s.size ();
    }
  }

  if (n <= 32766) // 32768 - "Unicode terminating null character".
    return auto_remove ();

  // Create the temporary file.
  //
  char d[MAX_PATH + 1], p[MAX_PATH + 1];
  if (GetTempPathA (sizeof (d), d) == 0 ||
      GetTempFileNameA (d, "odb-options-", 0, p) == 0)
  {
    print_error (name);
    throw process_failure ();
  }

  auto_remove rm = auto_remove (path (p));
  try
  {
    ofstream ofs (p);
    if (!ofs.is_open ())
    {
      cerr << name << ": error: unable to open '" << p << "' in write mode"
           << endl;
      throw process_failure ();
    }

    ofs.exceptions (ios_base::badbit | ios_base::failbit);

    // Write the arguments to file.
    //
    // The format is a space-separated list of potentially-quoted arguments
    // with support for backslash-escaping.
    //
    string b;
    for (size_t i (start), n (args.size () - 1); i != n; ++i)
    {
      const char* a (args[i]);

      // We will most likely have backslashes so just do it.
      //
      {
        for (b.clear (); *a != '\0'; ++a)
        {
          if (*a != '\\')
            b += *a;
          else
            b += "\\\\";
        }

        a = b.c_str ();
      }

      s.clear ();
      append_quoted (s, a);
      ofs << (i != start ? " " : "") << s;
    }

    ofs << '\n';
    ofs.close ();
  }
  catch (const ios_base::failure&)
  {
    cerr << name << ": error: unable to write to '" << p << "'" << endl;
    throw process_failure ();
  }

  // Rewrite the command line.
  //
  arg = string ("@") + p;
  args.resize (start);
  args.push_back (arg.c_str());
  args.push_back (nullptr);

  return rm;
}

static process_info
start_process (char const* args[], char const* name, bool err, bool out)
{
  HANDLE out_h[2];
  HANDLE in_eh[2];
  HANDLE in_oh[2];
  SECURITY_ATTRIBUTES sa;

  sa.nLength = sizeof (SECURITY_ATTRIBUTES);
  sa.bInheritHandle = true;
  sa.lpSecurityDescriptor = 0;

  if (!CreatePipe (&out_h[0], &out_h[1], &sa, 0) ||
      !SetHandleInformation (out_h[1], HANDLE_FLAG_INHERIT, 0))
  {
    print_error (name);
    throw process_failure ();
  }

  if (err)
  {
    if (!CreatePipe (&in_eh[0], &in_eh[1], &sa, 0) ||
        !SetHandleInformation (in_eh[0], HANDLE_FLAG_INHERIT, 0))
    {
      print_error (name);
      throw process_failure ();
    }
  }

  if (out)
  {
    if (!CreatePipe (&in_oh[0], &in_oh[1], &sa, 0) ||
        !SetHandleInformation (in_oh[0], HANDLE_FLAG_INHERIT, 0))
    {
      print_error (name);
      throw process_failure ();
    }
  }

  // Create the process.
  //
  path file (args[0]);

  // Do PATH search.
  //
  if (file.directory ().empty ())
    file = path_search (file);
  else if (file.base () == file) // No extension
    file += ".exe"; // Assume .exe.

  if (file.empty ())
  {
    cerr << args[0] << ": error: file not found" << endl;
    throw process_failure ();
  }

  // Serialize the arguments to string.
  //
  string cmd_line;

  for (char const** p (args); *p != 0; ++p)
  {
    if (p != args)
      cmd_line += ' ';

    append_quoted (cmd_line, *p);
  }

  // Prepare other info.
  //
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  memset (&si, 0, sizeof (STARTUPINFO));
  memset (&pi, 0, sizeof (PROCESS_INFORMATION));

  si.cb = sizeof(STARTUPINFO);

  if (err)
    si.hStdError = in_eh[1];
  else
    si.hStdError = GetStdHandle (STD_ERROR_HANDLE);

  if (out)
    si.hStdOutput = in_oh[1];
  else
    si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);

  si.hStdInput = out_h[0];
  si.dwFlags |= STARTF_USESTDHANDLES;

  if (!CreateProcess (
        file.string ().c_str (),
        const_cast<char*> (cmd_line.c_str ()),
        0,    // Process security attributes.
        0,    // Primary thread security attributes.
        true, // Inherit handles.
        0,    // Creation flags.
        0,    // Use our environment.
        0,    // Use our current directory.
        &si,
        &pi))
  {
    print_error (name);
    throw process_failure ();
  }

  CloseHandle (pi.hThread);
  CloseHandle (out_h[0]);

  if (err)
    CloseHandle (in_eh[1]);

  if (out)
    CloseHandle (in_oh[1]);

  process_info r;
  r.id = pi.hProcess;
  r.out_fd = _open_osfhandle ((intptr_t) (out_h[1]), 0);

  if (r.out_fd == -1)
  {
    cerr << name << ": error: unable to obtain C file handle" << endl;
    throw process_failure ();
  }

  if (err)
  {
    // Pass _O_TEXT to get newline translation.
    //
    r.in_efd = _open_osfhandle ((intptr_t) (in_eh[0]), _O_TEXT);

    if (r.in_efd == -1)
    {
      cerr << name << ": error: unable to obtain C file handle" << endl;
      throw process_failure ();
    }
  }
  else
    r.in_efd = 0;

  if (out)
  {
    // Pass _O_TEXT to get newline translation.
    //
    r.in_ofd = _open_osfhandle ((intptr_t) (in_oh[0]), _O_TEXT);

    if (r.in_ofd == -1)
    {
      cerr << name << ": error: unable to obtain C file handle" << endl;
      throw process_failure ();
    }
  }
  else
    r.in_ofd = 0;

  return r;
}

static bool
wait_process (process_info pi, char const* name)
{
  DWORD status;

  if (WaitForSingleObject (pi.id, INFINITE) != WAIT_OBJECT_0 ||
      !GetExitCodeProcess (pi.id, &status))
  {
    print_error (name);
    throw process_failure ();
  }

  CloseHandle (pi.id);
  return status == 0;
}

#endif // _WIN32
