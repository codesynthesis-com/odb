// file      : odb/details/win32/dll.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// If we are building a static library then omit DllMain.

#ifdef LIBODB_SHARED_BUILD

#include <odb/details/win32/windows.hxx>
#include <odb/details/win32/init.hxx>

using namespace odb::details;

extern "C" BOOL WINAPI
DllMain (HINSTANCE, DWORD reason, LPVOID reserved)
{
  switch (reason)
  {
  case DLL_PROCESS_ATTACH:
    {
      process_start ();
      thread_start ();
      break;
    }

  case DLL_THREAD_ATTACH:
    {
      thread_start ();
      break;
    }

  case DLL_THREAD_DETACH:
    {
      thread_end ();
      break;
    }

  case DLL_PROCESS_DETACH:
    {
      thread_end ();
      process_end (reserved == NULL);
      break;
    }
  }

  return 1;
}

#endif
