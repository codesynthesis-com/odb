// file      : relationship/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <vector>
#include <string>
#include <memory> // shared_ptr

#include <odb/core.hxx>

// The "pointer architecture" in this object model is as follows: All
// object pointers are eager. The employee class holds shared pointers
// to employer and projects.
//
// The following unidirectional relationships are used:
//
// to-one  : employee -> employer
// to-many : employee -> project
//

// Forward declarations.
//
class employer;
class project;
class employee;

typedef std::vector<std::shared_ptr<project>> projects;

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

private:
  friend class odb::access;

  employer () {}

  #pragma db id
  std::string name_;
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

private:
  friend class odb::access;

  project () {}

  #pragma db id
  std::string name_;
};

#pragma db object
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
  std::shared_ptr<employer_type> employer_;

  #pragma db value_not_null unordered
  projects_type projects_;
};

#endif // EMPLOYEE_HXX
