// file      : view/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <string>
#include <memory>  // std::shared_ptr
#include <cstddef> // std::size_t

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#pragma db object
class country
{
public:
  country (const std::string& code, const std::string& name)
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
            std::shared_ptr<country> res,
            std::shared_ptr<country> nat,
            std::shared_ptr<employer> e)
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

  // Age.
  //
  unsigned short
  age () const
  {
    return age_;
  }

  // Employer.
  //
  std::shared_ptr<employer>
  employed_by () const
  {
    return employed_by_;
  }

  // Residence and nationality.
  //
  std::shared_ptr<country>
  residence () const
  {
    return residence_;
  }

  std::shared_ptr<country>
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

  std::shared_ptr<country> residence_;
  std::shared_ptr<country> nationality_;

  std::shared_ptr<employer> employed_by_;
};

// We also have the "legacy" employee_extra table that is not mapped to any
// persistent class. It has the following columns:
//
// CREATE TABLE employee_extra(
//   employee_id INTEGER NOT NULL,
//   vacation_days INTEGER NOT NULL,
//   previous_employer_id INTEGER)
//

// A simple view with a single associated object. It allows us to get
// the name of an employee without loading any of the other parts, such
// as the referenced country and employer objects. The first and last
// data members in the view are automatically associated to the first_
// and last_ members in the employee object.
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
// name of an employee and its employer without loading any other parts.
// Because there is an unambiguous relationship between the employee and
// employer objects (employee::employed_by_), the ODB compiler is able to
// automatically use this relationship as a join condition. Also, similar
// to the employee_name view, the first and last data members are auto-
// associated to the first_ and last_ members in the employee object.
// For the employer_name member we provide an explicit member reference.
//
#pragma db view object(employee) object(employer)
struct employee_employer
{
  std::string first;
  std::string last;

  #pragma db column(employer::name_)
  std::string employer_name;
};

// A more interesting aggregate view that uses the GROUP BY clause. It
// allows us to calculate the min/max ages of employees for each employer.
// Here we use the query condition with a placeholder (?).
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
// of the same type. This requires us to use aliases and disambiguate
// the relationships used to associate each object.
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

// An example of an object loading view. It is a different version of
// the above view that loads the complete objects instead of a subset
// of their data members.
//
#pragma db view object(employee) \
  object(country = res: employee::residence_) \
  object(country = nat: employee::nationality_)
struct employee_country_objects
{
  std::shared_ptr<employee> e;
  std::shared_ptr<country> res;
  std::shared_ptr<country> nat;
};

// An example of using inner join type. Here we want to find employers
// that have any employees. If we were to use the default left join type,
// then we would have gotten all the employers, regardless of whether
// they have any employees. By using the inner join, we make sure that
// only matching employers are returned.
//
// It is also likely that there will be more than one employee for any
// particular employer which will lead to duplicate employer records
// being returned. To avoid this we use the 'distinct' result modifier.
//
// Try to change the join type or remove 'distinct' to observe the
// change in behavior.
//
#pragma db view object(employer) object(employee inner) query(distinct)
struct employer_with_employees
{
  std::shared_ptr<employer> e;
};

// An example of a native view that provides a complete query and is based
// on an ad-hoc table. This view allows us to load the employee vacation
// information from the legacy employee_extra table.
//
#ifndef ODB_DATABASE_ORACLE
  #pragma db view query("SELECT employee_id, vacation_days " \
                        "FROM view_employee_extra")
#else
  #pragma db view query("SELECT \"employee_id\", \"vacation_days\" " \
                        "FROM \"view_employee_extra\"")
#endif
struct employee_vacation
{
  #pragma db type("INTEGER")
  unsigned long id;

  #pragma db type("INTEGER")
  unsigned short days;
};

// A more robust implementation of the above view as a table view instead
// of a native view.
//
#pragma db view table("view_employee_extra")
struct employee_vacation1
{
  #pragma db column("employee_id") type("INTEGER")
  unsigned long id;

  #pragma db column("vacation_days") type("INTEGER")
  unsigned short days;
};

// An improved version of the previous view that extracts the employee
// first and last names instead of the id. To get the names we need to
// add the employee object to this view and use a custom join condition
// to tie it up with our legacy table.
//
#ifndef ODB_DATABASE_ORACLE
  #pragma db view table("view_employee_extra") \
    object(employee: "view_employee_extra.employee_id = " + employee::id_)
#else
  #pragma db view table("view_employee_extra") \
    object(employee: "\"view_employee_extra\".\"employee_id\" = " +  \
           employee::id_)
#endif
struct employee_vacation2
{
  std::string first;
  std::string last;

  #pragma db column("view_employee_extra.vacation_days") type("INTEGER")
  unsigned short days;
};

// A mixed view that associates two objects and a legacy table. It returns
// the previous employer information for each employee.
//
#ifndef ODB_DATABASE_ORACLE
  #pragma db view object(employee) \
    table("view_employee_extra" = "extra": \
          "extra.employee_id = " + employee::id_) \
    object(employer: "extra.previous_employer_id = " + employer::id_)
#else
  #pragma db view object(employee) \
    table("view_employee_extra" = "extra": \
          "\"extra\".\"employee_id\" = " + employee::id_) \
    object(employer: "\"extra\".\"previous_employer_id\" = " + employer::id_)
#endif
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
