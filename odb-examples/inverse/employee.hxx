// file      : inverse/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <vector>
#include <string>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

// The "pointer architecture" in this object model is as follows: All
// object pointers are lazy. The employee class holds shared pointers
// to employer, position, and projects. All other objects hold weak
// pointers back to the employee object. The weak sides are also the
// ones that are made inverse.
//
// The following bidirectional relationships are used:
//
// many-to-one  : employee <--> employer
// one-to-one   : employee <--> position
// many-to-many : employee <--> project
//

// Forward declarations.
//
class employer;
class position;
class project;
class employee;

typedef std::vector<odb::lazy_shared_ptr<project>> projects;
typedef std::vector<odb::lazy_weak_ptr<employee>> employees;

#pragma db object
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
  typedef ::employees employees_type;

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

#pragma db object
class position
{
public:
  position (const std::string& title)
      : title_ (title)
  {
  }

  const std::string&
  title () const
  {
    return title_;
  }

  // Employee that fills this position. NULL if the position is vacant.
  //
  typedef ::employee employee_type;

  const odb::lazy_weak_ptr<employee_type>&
  employee () const
  {
    return employee_;
  }

  void
  employee (odb::lazy_weak_ptr<employee_type> employee)
  {
    employee_ = employee;
  }

private:
  friend class odb::access;

  position () {}

  #pragma db id auto
  unsigned long id_;

  std::string title_;

  #pragma db inverse(position_)
  odb::lazy_weak_ptr<employee_type> employee_;
};

#pragma db object
class project
{
public:
  project (const std::string& name)
      : name_ (name)
  {
  }

  const std::string&
  name () const
  {
    return name_;
  }

  // Employees working on this project.
  //
  typedef ::employees employees_type;

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

  project () {}

  #pragma db id
  std::string name_;

  #pragma db value_not_null inverse(projects_)
  employees_type employees_;
};

#pragma db object
class employee
{
public:
  typedef ::employer employer_type;
  typedef ::position position_type;

  employee (const std::string& first,
            const std::string& last,
            odb::lazy_shared_ptr<employer_type> employer,
            odb::lazy_shared_ptr<position_type> position)
      : first_ (first), last_ (last),
        employer_ (employer),
        position_ (position)
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

  // Employer.
  //
  const odb::lazy_shared_ptr<employer_type>&
  employer () const
  {
    return employer_;
  }

  void
  employer (odb::lazy_shared_ptr<employer_type> employer)
  {
    employer_ = employer;
  }

  // Position.
  //
  const odb::lazy_shared_ptr<position_type>&
  position () const
  {
    return position_;
  }

  void
  position (odb::lazy_shared_ptr<position_type> position)
  {
    position_ = position;
  }

  // Projects.
  //
  typedef ::projects projects_type;

  const projects_type&
  projects () const
  {
    return projects_;
  }

  projects_type&
  projects ()
  {
    return projects_;
  }

private:
  friend class odb::access;

  employee () {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;

  #pragma db not_null
  odb::lazy_shared_ptr<employer_type> employer_;

  #pragma db not_null
  odb::lazy_shared_ptr<position_type> position_;

  #pragma db value_not_null unordered
  projects_type projects_;
};

#endif // EMPLOYEE_HXX
