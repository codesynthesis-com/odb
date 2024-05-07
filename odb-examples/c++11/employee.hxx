// file      : c++11/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <string>
#include <memory> // std::unique_ptr, std::shared_ptr
#include <vector>
#include <unordered_set>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx> // odb::lazy_shared_ptr, odb::lazy_weak_ptr

// Forward declarations.
//
class employer;
class employee;

#pragma db object pointer(std::shared_ptr) session
class employer
{
public:
  employer (const std::string& name)
      : name_ (name)
  {
  }

  const std::string&
  name () const
  {
    return name_;
  }

  // Employees of this employer.
  //
  typedef std::vector<odb::lazy_weak_ptr<employee>> employees_type;

  const employees_type&
  employees () const
  {
    return employees_;
  }

  employees_type&
  employees ()
  {
    return employees_;
  }

private:
  friend class odb::access;

  employer () {}

  #pragma db id
  std::string name_;

  #pragma db value_not_null inverse(employer_)
  employees_type employees_;
};

#pragma db object pointer(std::shared_ptr) session
class employee
{
public:
  typedef ::employer employer_type;

  employee (const std::string& first,
            const std::string& last,
            std::shared_ptr<employer_type> employer)
      : first_ (first), last_ (last), employer_ (employer)
  {
  }

  // Name.
  //
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

  // Emails.
  //
  typedef std::unordered_set<std::string> emails_type;

  const emails_type&
  emails () const
  {
    return emails_;
  }

  emails_type&
  emails ()
  {
    return emails_;
  }

  // Employer.
  //
  std::shared_ptr<employer_type>
  employer () const
  {
    return employer_;
  }

  void
  employer (std::shared_ptr<employer_type> employer)
  {
    employer_ = employer;
  }

private:
  friend class odb::access;

  employee () {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;
  emails_type emails_;

  #pragma db not_null
  std::shared_ptr<employer_type> employer_;
};

// std::unique_ptr is a good choice for an object pointer if we are
// not planning to do any sharing.
//
#pragma db object pointer(std::unique_ptr)
class pension_fund
{
public:
  pension_fund (const std::string& name)
      : name_ (name)
  {
  }

  const std::string&
  name () const
  {
    return name_;
  }

  // Members of this fund.
  //
  typedef std::vector<odb::lazy_shared_ptr<employee>> members_type;

  const members_type&
  members () const
  {
    return members_;
  }

  members_type&
  members ()
  {
    return members_;
  }

private:
  friend class odb::access;

  pension_fund () {}

  #pragma db id
  std::string name_;

  #pragma db value_not_null
  members_type members_;
};

#endif // EMPLOYEE_HXX
