// file      : common/session/custom/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <memory>
#include <vector>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

class employee;

#pragma db object pointer(std::shared_ptr) session
class employer
{
public:
  employer (const std::string& name, const std::string& symbol)
      : name_ (name), symbol_ (symbol) {}

  const std::string&
  name () const {return name_;}

  const std::string&
  symbol () const {return symbol_;}

  void
  symbol (const std::string& symbol) {symbol_ = symbol;}

  // Employees of this employer.
  //
  typedef std::vector<odb::lazy_weak_ptr<employee>> employees_type;

  const employees_type&
  employees () const {return employees_;}

  employees_type&
  employees () {return employees_;}

  // Change tracking.
  //
public:
  bool
  changed (const employer& orig) const
  {
    // Note that we don't need to track object ids, inverse pointers, nor
    // readonly/const data members.
    //
    return symbol_ != orig.symbol_;
  }

private:
  friend class odb::access;
  employer () {}

  #pragma db id
  std::string name_;

  std::string symbol_;

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
      : first_ (first), last_ (last), employer_ (employer) {}

  // Name.
  //
  const std::string&
  first () const {return first_;}

  const std::string&
  last () const {return last_;}

  // Employer.
  //
  std::shared_ptr<employer_type>
  employer () const {return employer_;}

  void
  employer (std::shared_ptr<employer_type> e) {employer_ = e;}

  // Change tracking.
  //
public:
  bool
  changed (const employee& orig) const
  {
    return first_ != orig.first_ || last_ != orig.last_ ||
      employer_ != orig.employer_;
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
};

#endif // TEST_HXX
