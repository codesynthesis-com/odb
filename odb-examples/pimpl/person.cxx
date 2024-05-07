// file      : pimpl/person.cxx
// copyright : not copyrighted - public domain

#include "person.hxx"

using namespace std;

struct person::impl
{
  impl () {}
  impl (const string& e, const string& n, unsigned short a)
      : email (e), name (n), age (a) {}

  string email;
  string name;
  unsigned short age;
};

person::
~person ()
{
  delete pimpl_;
}

person::
person ()
    : pimpl_ (new impl)
{
}

person::
person (const string& e, const string& n, unsigned short a)
    : pimpl_ (new impl (e, n, a))
{
}

const string& person::
email () const
{
  return pimpl_->email;
}

void person::
email (const string& e)
{
  pimpl_->email = e;
}

const string& person::
name () const
{
  return pimpl_->name;
}

void person::
name (const string& n)
{
  pimpl_->name = n;
}

unsigned short person::
age () const
{
  return pimpl_->age;
}

void person::
age (unsigned short a) const
{
  pimpl_->age = a;
}
