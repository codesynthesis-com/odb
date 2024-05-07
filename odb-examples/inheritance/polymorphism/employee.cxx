// file      : inheritance/polymorphism/employee.cxx
// copyright : not copyrighted - public domain

#include <iostream>

#include "employee.hxx"

using namespace std;

person::
~person ()
{
}

void employee::
print ()
{
  cout << first_ << ' ' << last_
       << (temporary_ ? " temporary " : " permanent ")
       << "employee" << endl;
}

void contractor::
print ()
{
  cout << first_ << ' ' << last_ << ' ' << email_ << " contractor" << endl;
}
