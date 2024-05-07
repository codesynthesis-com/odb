// file      : boost/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/unordered_set.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <odb/core.hxx>

#include <odb/boost/lazy-ptr.hxx>

using boost::shared_ptr;
using boost::weak_ptr;

using odb::boost::lazy_shared_ptr;
using odb::boost::lazy_weak_ptr;

using boost::uuids::uuid;
using boost::gregorian::date;

// Forward declarations.
//
class employer;
class employee;

typedef boost::unordered_set<std::string> emails;
typedef std::vector<lazy_weak_ptr<employee> > employees;

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
class employee
{
public:
  typedef ::employer employer_type;

  employee (const std::string& first,
            const std::string& last,
            const date& born,
            shared_ptr<employer_type> employer)
      : id_ (boost::uuids::random_generator () ()),
        first_ (first), last_ (last),
        born_ (born),
        employer_ (employer)
  {
  }

  employee (const std::string& first,
            const std::string& middle,
            const std::string& last,
            const date& born,
            shared_ptr<employer_type> employer)
      : id_ (boost::uuids::random_generator () ()),
        first_ (first), middle_ (middle), last_ (last),
        born_ (born),
        employer_ (employer)
  {
  }

  // Id.
  //
  const uuid&
  id () const
  {
    return id_;
  }

  // Name.
  //
  const std::string&
  first () const
  {
    return first_;
  }

  const boost::optional<std::string>&
  middle () const
  {
    return middle_;
  }

  const std::string&
  last () const
  {
    return last_;
  }

  // Date of birth.
  //
  const date&
  born () const
  {
    return born_;
  }

  // Emails.
  //
  typedef ::emails emails_type;

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
  shared_ptr<employer_type>
  employer () const
  {
    return employer_;
  }

  void
  employer (shared_ptr<employer_type> employer)
  {
    employer_ = employer;
  }

private:
  friend class odb::access;

  employee () {}

  #pragma db id
  uuid id_;

  std::string first_;
  boost::optional<std::string> middle_;
  std::string last_;

  date born_;
  emails_type emails_;

  #pragma db not_null
  shared_ptr<employer_type> employer_;
};

#endif // EMPLOYEE_HXX
