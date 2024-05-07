// file      : schema/custom/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <vector>
#include <string>
#include <memory> // std::shared_ptr

#include <odb/core.hxx>

typedef std::vector<std::string> degrees;

#pragma db value
class name
{
public:
  name (const std::string& first, const std::string& last)
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

private:
  friend class odb::access;

  #pragma db type("VARCHAR(255)") column("first_name")
  std::string first_;

  #pragma db type("VARCHAR(255)") column("last_name")
  std::string last_;
};

#pragma db object table("Employer")
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

  #pragma db id type("VARCHAR(255)") column("name")
  std::string name_;
};

#pragma db object table("Employee")
class employee
{
public:
  typedef ::employer employer_type;

  employee (unsigned long id,
            const std::string& first,
            const std::string& last,
            std::shared_ptr<employer_type> employer)
      : id_ (id), name_ (first, last), employer_ (employer)
  {
  }

  // Name.
  //
  typedef ::name name_type;

  const name_type&
  name () const
  {
    return name_;
  }

  // Degrees.
  //
  typedef ::degrees degrees_type;

  const degrees_type&
  degrees () const
  {
    return degrees_;
  }

  degrees_type&
  degrees ()
  {
    return degrees_;
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

  employee (): name_ ("", "") {}

  #pragma db id type("INTEGER") column("ssn")
  unsigned long id_;

  #pragma db column("") // No column prefix.
  name_type name_;

  #pragma db unordered table("EmployeeDegree") id_column("ssn") \
    value_type("VARCHAR(255)") value_column("degree")
  degrees_type degrees_;

  #pragma db not_null column("employer")
  std::shared_ptr<employer_type> employer_;
};

#endif // EMPLOYEE_HXX
