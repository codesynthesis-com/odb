// file      : pgsql/template/driver.cxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// PLACE TEST DESCRIPTION HERE
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

using namespace std;

int
main ()
{
  try
  {
    cout << "test 001" << endl;
  }
  catch (...)
  {
    return 1;
  }
}
