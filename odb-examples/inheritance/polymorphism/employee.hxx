// file      : inheritance/polymorphism/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object polymorphic
class person
{
public:
  person (const std::string& first, const std::string& last)
      : first_ (first), last_ (last)
  {
  }

  const std::string&
  first () const
  {
    return first_;
  }

  const std::string&
  last () const
  {
    return last_;
  }

  virtual
  ~person () = 0;

  virtual void
  print () = 0;

protected:
  friend class odb::access;
  person () {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;
};

#pragma db object
class employee: public person
{
public:
  employee (const std::string& first,
            const std::string& last,
            bool temporary)
      : person (first, last), temporary_ (temporary)
  {
  }

  bool
  temporary () const
  {
    return temporary_;
  }

  void
  temporary (bool t)
  {
    temporary_ = t;
  }

  virtual void
  print ();

private:
  friend class odb::access;
  employee () {}

  bool temporary_;
};

#pragma db object
class contractor: public person
{
public:
  contractor (const std::string& first,
              const std::string& last,
              const std::string& email)
      : person (first, last), email_ (email)
  {
  }

  const std::string&
  email () const
  {
    return email_;
  }

  virtual void
  print ();

private:
  friend class odb::access;
  contractor () {}

  std::string email_;
};

#endif // EMPLOYEE_HXX
