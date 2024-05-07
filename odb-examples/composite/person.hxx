// file      : composite/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <vector>
#include <string>

#include <odb/core.hxx>

#pragma db value
class basic_name
{
public:
  basic_name (const std::string& first, const std::string& last)
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

  basic_name () {} // Needed for storing basic_name in containers.

  std::string first_;
  std::string last_;
};

typedef std::vector<basic_name> basic_names;


#pragma db value
class name_extras
{
public:
  // Nickname.
  //
  const std::string&
  nickname () const
  {
    return nickname_;
  }

  void
  nickname (const std::string& nickname)
  {
    nickname_ = nickname;
  }

  // Aliases.
  //
  const basic_names&
  aliases () const
  {
    return aliases_;
  }

  basic_names&
  aliases ()
  {
    return aliases_;
  }

private:
  friend class odb::access;

  std::string nickname_;
  basic_names aliases_;
};


#pragma db value
class name: public basic_name
{
public:
  name (const std::string& first,
        const std::string& last,
        const std::string& title)
      : basic_name (first, last), title_ (title)
  {
  }

  // Title.
  //
  const std::string&
  title () const
  {
    return title_;
  }

  // Extras.
  //
  const name_extras&
  extras () const
  {
    return extras_;
  }

  name_extras&
  extras ()
  {
    return extras_;
  }

private:
  friend class odb::access;

  std::string title_;
  name_extras extras_;
};

// We can also define a composite value type as a class template
// instantiation. Here we use std::pair to store person's phone
// numbers, in the order of preference.
//
typedef std::pair<std::string, std::string> phone_numbers;
#pragma db value(phone_numbers)

// We can also use a composite value type as an object id.
//
#pragma db value
class email_address
{
public:
  email_address () {}
  email_address (const std::string& address)
  {
    std::string::size_type p (address.find ('@'));
    recipient_.assign (address, 0, p);
    domain_.assign (address, p + 1, std::string::npos);
  }

  const std::string&
  recipient () const
  {
    return recipient_;
  }

  const std::string&
  domain () const
  {
    return domain_;
  }

  std::string
  address () const
  {
    return recipient_ + '@' + domain_;
  }

private:
  friend class odb::access;

  std::string recipient_;
  std::string domain_;
};

#pragma db object
class person
{
public:
  person (const std::string& email,
          const std::string& first,
          const std::string& last,
          const std::string& title,
          const phone_numbers& phone)
      : email_ (email), name_ (first, last, title), phone_ (phone)
  {
  }

  // Email address.
  //
  const email_address&
  email () const
  {
    return email_;
  }

  // Name.
  //
  typedef ::name name_type;

  const name_type&
  name () const
  {
    return name_;
  }

  name_type&
  name ()
  {
    return name_;
  }

  // Phone.
  //
  const phone_numbers&
  phone () const
  {
    return phone_;
  }

private:
  friend class odb::access;

  person (): name_ ("", "", "") {}

  #pragma db id
  email_address email_;

  name_type name_;
  phone_numbers phone_;
};

#endif // PERSON_HXX
