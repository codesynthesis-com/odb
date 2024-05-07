// file      : qt/employee.hxx
// copyright : not copyrighted - public domain

#ifndef EMPLOYEE_HXX
#define EMPLOYEE_HXX

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QUuid>
#include <QtCore/QDateTime>
#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <QtCore/QSet>

#include <odb/core.hxx>

#include <odb/qt/lazy-ptr.hxx>

// Forward declarations.
//
class Employee;

typedef QSet<QString> Emails;
typedef QList<QLazyWeakPointer<Employee> > Employees;

#pragma db object
class Employer
{
public:

  Employer (const QString& name)
      : name_ (name)
  {
  }

  const QString&
  name () const
  {
    return name_;
  }

  const Employees&
  employees () const
  {
    return employees_;
  }

  Employees&
  employees ()
  {
    return employees_;
  }

private:
  friend class odb::access;

  Employer () {}

  #pragma db id
  QString name_;

  #pragma db value_not_null inverse(employer_)
  Employees employees_;
};

#pragma db object
class Employee
{
public:

  Employee (const QString& first,
            const QString& last,
            const QDate& born,
            const QByteArray& publicKey,
            QSharedPointer<Employer> employer)
      : id_ (QUuid::createUuid ()),
        first_ (first),
        last_ (last),
        born_ (born),
        publicKey_ (publicKey),
        employer_ (employer)
  {
  }

  // Id.
  //
  const QUuid&
  id () const
  {
    return id_;
  }

  // Name.
  //
  const QString&
  first () const
  {
    return first_;
  }

  const QString&
  last () const
  {
    return last_;
  }

  // Date of birth.
  //
  const QDate&
  born () const
  {
    return born_;
  }

  const Emails&
  emails () const
  {
    return emails_;
  }

  Emails&
  emails ()
  {
    return emails_;
  }

  // Public key.
  //
  const QByteArray&
  publicKey () const
  {
    return publicKey_;
  }

  // Employer.
  //
  QLazySharedPointer<Employer>
  employer () const
  {
    return employer_;
  }

  void
  employer (QSharedPointer<Employer> employer)
  {
    employer_ = employer;
  }

private:
  friend class odb::access;

  Employee () {}

  #pragma db id
  QUuid id_;

  QString first_;
  QString last_;
  QDate born_;
  Emails emails_;
  QByteArray publicKey_;

  #pragma db not_null
  QLazySharedPointer<Employer> employer_;
};

#endif // EMPLOYEE_HXX
