// file      : odb/plugin.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx> // Keep it first.

#include <unistd.h>    // stat()
#include <sys/types.h> // stat
#include <sys/stat.h>  // stat

#include <memory>  // std::unique_ptr
#include <string>
#include <vector>
#include <cstring> // std::strcpy, std::strstr
#include <cassert>
#include <iostream>

#include <libcutl/re.hxx>
#include <libcutl/fs/path.hxx>

#include <odb/pragma.hxx>
#include <odb/parser.hxx>
#include <odb/options.hxx>
#include <odb/option-functions.hxx>
#include <odb/features.hxx>
#include <odb/profile.hxx>
#include <odb/version.hxx>
#include <odb/validator.hxx>
#include <odb/processor.hxx>
#include <odb/generator.hxx>
#include <odb/semantics/unit.hxx>

using namespace std;
using namespace semantics;

using cutl::fs::path;
using cutl::fs::invalid_path;

typedef vector<path> paths;

#if defined(_WIN32) && !defined(ODB_STATIC_PLUGIN)
__declspec(dllexport)
#endif
int plugin_is_GPL_compatible;

unique_ptr<options const> options_;
paths profile_paths_;
path file_;    // File being compiled.
paths inputs_; // List of input files in at-once mode or just file_.

bool (*cpp_diagnostic_prev) (
  cpp_reader*,
#if BUILDING_GCC_MAJOR >= 9
  cpp_diagnostic_level,
  cpp_warning_reason,
#else
  int,
  int,
#endif
#if BUILDING_GCC_MAJOR >= 6
  rich_location*,
#else
  location_t,
  unsigned int,
#endif
  const char*,
  va_list*);

static bool
cpp_diagnostic_filter (cpp_reader* r,
#if BUILDING_GCC_MAJOR >= 9
                       cpp_diagnostic_level level,
                       cpp_warning_reason reason,
#else
                       int level,
                       int reason,
#endif
#if BUILDING_GCC_MAJOR >= 6
                       rich_location* l,
#else
                       location_t l,
                       unsigned int column_override,
#endif
                       const char* msg,
                       va_list* ap)
{
  // #pragma once in the main file. Note that the message that we get is
  // potentially translated so we search for the substring (there is
  // currently only one warning in libcpp that mentions #pragma once).
  // Unfortunately, some translations translate the 'once' word (e.g,
  // #pragma uno; I wonder if one can actually specify it like that in
  // the source code). Oh, well, there is only so much we can do.
  //
  if (strstr (msg, "#pragma once") != 0)
    return true;

  return cpp_diagnostic_prev (
    r,
    level,
    reason,
#if BUILDING_GCC_MAJOR >= 6
    l,
#else
    l,
    column_override,
#endif
    msg,
    ap);
}

// A prefix of the _cpp_file struct. This struct is not part of the
// public interface so we have to resort to this technique (based on
// libcpp/files.c).
//
struct cpp_file_prefix
{
  char const* name;
  char const* path;
  char const* pchname;
  char const* dir_name;
  _cpp_file* next_file;
  const uchar* buffer;
  const uchar* buffer_start;
  const cpp_hashnode *cmacro;
  cpp_dir *dir;
  struct stat st;
};

extern "C" void
start_unit_callback (void*, void*)
{
  // Set the preprocessor error callback to filter out useless diagnostics.
  //
  cpp_callbacks* cb (cpp_get_callbacks (parse_in));

#if BUILDING_GCC_MAJOR >= 9
  cpp_diagnostic_prev = cb->diagnostic;
  cb->diagnostic = &cpp_diagnostic_filter;
#else
  cpp_diagnostic_prev = cb->error;
  cb->error = &cpp_diagnostic_filter;
#endif

  if (cpp_diagnostic_prev == 0)
  {
    cerr << "ice: expected cpp diagnostic callback to be set" << endl;
    exit (1);
  }

  // Set the directory of the main file (stdin) to that of the orginal
  // file so that relative inclusion works. Also adjust the path and
  // re-stat the file so that #pragma once works.
  //
  cpp_buffer* b (cpp_get_buffer (parse_in));
  _cpp_file* f (cpp_get_file (b));
  cpp_dir* d (cpp_get_dir (f));
  char const* p (cpp_get_path (f));

  cpp_file_prefix* fp (reinterpret_cast<cpp_file_prefix*> (f));

  // Perform some sanity checks.
  //
  if (p != 0 && *p == '\0'     // The path should be empty (stdin).
      && cpp_get_prev (b) == 0 // This is the only buffer (main file).
      && fp->path == p         // Our prefix corresponds to the actual type.
      && fp->dir == d          // Our prefix corresponds to the actual type.
      && fp->dir_name == 0)    // The directory part hasn't been initialized.
  {
    // The dir_name is initialized by libcpp lazily so we can preemptively
    // set it to what we need.
    //
    path d (file_.directory ());
    char* s;

    if (d.empty ())
    {
      s = XNEWVEC (char, 1);
      *s = '\0';
    }
    else
    {
      size_t n (d.string ().size ());
      s = XNEWVEC (char, n + 2);
      strcpy (s, d.string ().c_str ());
      s[n] = path::traits::directory_separator;
      s[n + 1] = '\0';
    }

    fp->dir_name = s;

    // Unless we are in the at-once mode (where input files are actually
    // #include'ed into the synthesized stdin), pretend that we are the
    // actual input file. This is necessary for the #pragma once to work.
    //
    // All this relies on the way things are implemented in libcpp. In
    // particular, the #pragma once code first checks if the mtime and
    // size of files match (that's why we need stat() below). If they
    // do, then it goes ahead and compares their contents. To re-load
    // the contents of the file libcpp uses the path (that's why we
    // need to adjust that as well).
    //
    if (inputs_.size () == 1)
    {
      string const& f (file_.string ());

      XDELETEVEC (fp->path);
      size_t n (f.size ());
      char* p (XNEWVEC (char, n + 1));
      strcpy (p, f.c_str ());
      p[n] = '\0';
      fp->path = p;

      // This call shouldn't fail since we've just opened it in the driver.
      stat (fp->path, &fp->st);
    }
  }
  else
  {
    cerr << "ice: unable to initialize main file directory" << endl;
    exit (1);
  }
}

extern "C" void
gate_callback (void*, void*)
{
  // If there were errors during compilation, let GCC handle the
  // exit.
  //
  if (errorcount || sorrycount)
    return;

  int r (0);

  try
  {
    // Post process pragmas.
    //
    post_process_pragmas ();

    // Parse the GCC tree to semantic graph.
    //
    parser p (*options_, loc_pragmas_, ns_loc_pragmas_, decl_pragmas_);
    unique_ptr<unit> u (p.parse (global_namespace, file_));

    features f;

    // Process, pass 1.
    //
    process (*options_, f, *u, file_, 1);

    // Validate, pass 1.
    //
    validate (*options_, f, *u, file_, 1);

    // Process, pass 2.
    //
    process (*options_, f, *u, file_, 2);

    // Validate, pass 2.
    //
    validate (*options_, f, *u, file_, 2);

    // Generate.
    //
    generate (*options_, f, *u, file_, inputs_);
  }
  catch (cutl::re::format const& e)
  {
    cerr << "error: invalid regex: '" << e.regex () << "': " <<
      e.description () << endl;
    r = 1;
  }
  catch (pragmas_failed const&)
  {
    // Diagnostics has aready been issued.
    //
    r = 1;
  }
  catch (parser::failed const&)
  {
    // Diagnostics has aready been issued.
    //
    r = 1;
  }
  catch (validator_failed const&)
  {
    // Diagnostics has aready been issued.
    //
    r = 1;
  }
  catch (processor_failed const&)
  {
    // Diagnostics has aready been issued.
    //
    r = 1;
  }
  catch (generator_failed const&)
  {
    // Diagnostics has aready been issued.
    //
    r = 1;
  }

  exit (r);
}

static char const* const odb_version = ODB_COMPILER_VERSION_STR;

typedef vector<string> strings;

extern "C"
#if defined(_WIN32) && !defined(ODB_STATIC_PLUGIN)
__declspec(dllexport)
#endif
int
plugin_init (plugin_name_args* plugin_info, plugin_gcc_version*)
{
  int r (0);
  plugin_info->version = odb_version;

  try
  {
    // Parse options.
    //
    {
      strings argv_str;
      argv_str.push_back (plugin_info->base_name);

      for (int i (0); i < plugin_info->argc; ++i)
      {
        plugin_argument& a (plugin_info->argv[i]);

        // A value cannot contain '=' so it is passed as the backspace
        // character.
        //
        string v (a.value != 0 ? a.value : "");
        for (size_t i (0); i < v.size (); ++i)
          if (v[i] == '\b')
            v[i] = '=';

        // Handle service options.
        //
        if (strcmp (a.key, "svc-path") == 0)
        {
          profile_paths_.push_back (path (v));
          continue;
        }

        if (strcmp (a.key, "svc-file") == 0)
        {
          // First is the main file. Subsequent are inputs in the at-once
          // mode.
          //
          if (file_.empty ())
            file_ = path (v);
          else
            inputs_.push_back (path (v));

          continue;
        }

        string opt (strlen (a.key) > 1 ? "--" : "-");
        opt += a.key;

        argv_str.push_back (opt);

        if (a.value != 0)
          argv_str.push_back (v);
      }

      vector<char*> argv;
      for (strings::iterator i (argv_str.begin ()); i != argv_str.end (); ++i)
        argv.push_back (const_cast<char*> (i->c_str ()));

      int argc (static_cast<int> (argv.size ()));

      if (inputs_.empty ())
        inputs_.push_back (file_);

      // Two-phase options parsing, similar to the driver.
      //
      cli::argv_file_scanner::option_info oi[3];
      oi[0].option = "--options-file";
      oi[0].search_func = 0;
      oi[1].option = "-p";
      oi[2].option = "--profile";

      database db;
      {
        oi[1].search_func = &profile_search_ignore;
        oi[2].search_func = &profile_search_ignore;

        cli::argv_file_scanner scan (argc, &argv[0], oi, 3);
        options ops (scan);
        assert (ops.database_specified ());
        db = ops.database ()[0];
      }

      profile_data pd (profile_paths_, db, "odb plugin");
      oi[1].search_func = &profile_search;
      oi[2].search_func = &profile_search;
      oi[1].arg = &pd;
      oi[2].arg = &pd;

      cli::argv_file_scanner scan (argc, &argv[0], oi, 3);
      unique_ptr<options> ops (
        new options (scan, cli::unknown_mode::fail, cli::unknown_mode::fail));

      // Process options.
      //
      process_options (*ops);

      options_ = move (ops);
      pragma_db_ = db;
      pragma_multi_ = options_->multi_database ();
    }

    if (options_->trace ())
      cerr << "starting plugin " << plugin_info->base_name << endl;

    // Disable assembly output. GCC doesn't define HOST_BIT_BUCKET
    // correctly for MinGW (it still used /dev/null which fails to
    // open).
    //
#ifdef _WIN32
    asm_file_name = "nul";
#else
    asm_file_name = HOST_BIT_BUCKET;
#endif

    // Register callbacks.
    //
    register_callback (plugin_info->base_name,
                       PLUGIN_PRAGMAS,
                       register_odb_pragmas,
                       0);

    register_callback (plugin_info->base_name,
                       PLUGIN_START_UNIT,
                       start_unit_callback,
                       0);

    register_callback (plugin_info->base_name,
                       PLUGIN_OVERRIDE_GATE,
                       &gate_callback,
                       0);
  }
  catch (cli::exception const& ex)
  {
    cerr << ex << endl;
    r = 1;
  }

  if (r != 0)
    exit (r);

  return r;
}
