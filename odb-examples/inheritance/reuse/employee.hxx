// file      : inheritance/reuse/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <string>

#include <odb/core.hxx>

// Abstract person class. Note that it does not declare the object id.
//
#pragma db object abstract
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

protected:
  friend class odb::access;
  person () {}

private:
  std::string first_;
  std::string last_;
};

// Abstract employee class. It derives from the person class and declares
// the object id for all the concrete employee types.
//
#pragma db object abstract
class employee: public person
{
public:
  employee (const std::string& first, const std::string& last)
      : person (first, last)
  {
  }

  unsigned long
  number () const
  {
    return id_;
  }

protected:
  friend class odb::access;
  employee () {}

private:
  #pragma db id auto
  unsigned long id_;
};

// Concrete permanent_employee class. Note that it doesn't define any
// data members of its own.
//
#pragma db object
class permanent_employee: public employee
{
public:
  permanent_employee (const std::string& first, const std::string& last)
      : employee (first, last)
  {
  }

private:
  friend class odb::access;
  permanent_employee () {}
};

// Concrete temporary_employee class. It adds the employment duration in
// months.
//
#pragma db object
class temporary_employee: public employee
{
public:
  temporary_employee (const std::string& first,
                      const std::string& last,
                      unsigned long duration)
      : employee (first, last), duration_ (duration)
  {
  }

  unsigned long
  duration () const
  {
    return duration_;
  }

private:
  friend class odb::access;
  temporary_employee () {}

  unsigned long duration_;
};

// Concrete contractor class. It derives from the person class (and not
// employee; an independent contractor is not considered an employee).
// We use the contractor's external email address as the object id.
//
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

private:
  friend class odb::access;
  contractor () {}

  #pragma db id
  std::string email_;
};

#endif // EMPLOYEE_HXX
