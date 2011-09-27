// file      : view/employee.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <string>
#include <cstddef> // std::size_t

#include <odb/core.hxx>
#include <odb/nullable.hxx>

// Include TR1 <memory> header in a compiler-specific fashion. Fall back
// on the Boost implementation if the compiler does not support TR1.
//
#include <odb/tr1/memory.hxx>

using std::tr1::shared_ptr;

#pragma db object
class country
{
public:
  country (const std::string& code, std::string const& name)
      : code_ (code), name_ (name)
  {
  }

  const std::string&
  name () const
  {
    return name_;
  }

private:
  friend class odb::access;

  country () {}

  #pragma db id
  std::string code_; // ISO 2-letter country code.

  std::string name_;
};

#pragma db object
class employer
{
public:
  employer (unsigned long id, const std::string& name)
      : id_ (id), name_ (name)
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
  unsigned long id_;

  std::string name_;
};

#pragma db object
class employee
{
public:
  employee (unsigned long id,
            const std::string& first,
            const std::string& last,
            unsigned short age,
            shared_ptr<country> res,
            shared_ptr<country> nat,
            shared_ptr<employer> e)
      : id_ (id), first_ (first), last_ (last), age_ (age),
        residence_ (res),
        nationality_ (nat),
        employed_by_ (e)
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
  shared_ptr<employer>
  employed_by () const
  {
    return employed_by_;
  }

  // Residence and nationality.
  //
  shared_ptr<country>
  residence () const
  {
    return residence_;
  }

  shared_ptr<country>
  nationality () const
  {
    return nationality_;
  }

private:
  friend class odb::access;

  employee () {}

  #pragma db id
  unsigned long id_;

  std::string first_;
  std::string last_;

  unsigned short age_;

  shared_ptr<country> residence_;
  shared_ptr<country> nationality_;

  shared_ptr<employer> employed_by_;
};

// We also have a "legacy" employee_extra table that is not mapped to any
// object. It has the following columns:
//
// CREATE TABLE employee_extra(
//   employee_id INTEGER NOT NULL,
//   vacation_days INTEGER NOT NULL,
//   previous_employer_id INTEGER)
//

// A simple view with a single associated object. It allows us to get
// the name of an employee without loading any of the other data, such
// as the country and employer objects. The first and last data members
// in the view are automatically assumed to correspond to the first_ and
// last_ members in the employee object.
//
#pragma db view object(employee)
struct employee_name
{
  std::string first;
  std::string last;
};

// A simple aggregate view. It allows us to count the number of employees
// matching certain criteria. Here we use a column expression with a
// reference to the id_ data member in the employee object.
//
#pragma db view object(employee)
struct employee_count
{
  #pragma db column("count(" + employee::id_ + ")")
  std::size_t count;
};

// A simple view with two associated object. It allows us to get the
// name of an employee and its employer without loading any other data.
// Because there is a relationship between the employee and employer
// objects (employee::employed_by_), the ODB compiler automatically
// used this relationship as a join condition. Also, similar to the
// employee_name view, the first and last data members are automatically
// assumed to correspond to the first_ and last_ members in the employee
// object. For the employer_name member we provide an explicit member
// reference.
//
#pragma db view object(employee) object(employer)
struct employee_employer
{
  std::string first;
  std::string last;

  #pragma db column(employer::name_)
  std::string employer_name;
};

// A more interesting aggregate view using GROUP BY. It allows us to
// calculate the min/max age of employees for each employer. Here we
// use the C++-integrated syntax for the query condition template with
// a placeholder (?).
//
#pragma db view object(employee) object(employer) \
  query ((?) + "GROUP BY" + employer::name_)
struct employer_age
{
  #pragma db column(employer::name_)
  std::string employer_name;

  #pragma db column("min(" + employee::age_ + ")")
  unsigned short min_age;

  #pragma db column("max(" + employee::age_ + ")")
  unsigned short max_age;
};

// A more complex view with three associated objects, two of which are
// of the same type, which requires us to use aliases and disambiguate
// the relationships used to join each object.
//
#pragma db view object(employee) \
  object(country = res_country: employee::residence_) \
  object(country = nat_country: employee::nationality_)
struct employee_country
{
  std::string first;
  std::string last;

  #pragma db column(res_country::name_)
  std::string res_country_name;

  #pragma db column(nat_country::name_)
  std::string nat_country_name;
};

// A native view. A native view provides a complete query and is normally
// based on an ad-hoc table. This view allows us to load the employee
// vacation information from the legacy employee_extra table.
//
#pragma db view query("SELECT employee_id, vacation_days " \
                      "FROM view_employee_extra")
struct employee_vacation
{
  #pragma db type("INTEGER")
  unsigned long id;

  #pragma db type("INTEGER")
  unsigned short days;
};

// An improved version of the previous view that extracts the employee
// first and last names instead of the id. To get the names we need to
// add the employee object to this view and use a custom join condition
// to tie it up with our legacy table.
//
#pragma db view table("view_employee_extra") \
  object(employee: "view_employee_extra.employee_id = " + employee::id_)
struct employee_vacation2
{
  std::string first;
  std::string last;

  #pragma db column("view_employee_extra.vacation_days") type("INTEGER")
  unsigned short days;
};

// An advanced view that joins two objects via a legacy table. It returns
// the previous employer information for each employee.
//
#pragma db view object(employee) \
  table("view_employee_extra" = "extra": \
        "extra.employee_id = " + employee::id_) \
  object(employer: "extra.previous_employer_id = " + employer::id_)
struct employee_prev_employer
{
  std::string first;
  std::string last;

  // If previous_employer_id is NULL, then the name will be NULL as well.
  // We use the odb::nullable wrapper to handle this.
  //
  #pragma db column(employer::name_)
  odb::nullable<std::string> prev_employer_name;
};

#endif // EMPLOYEE_HXX
