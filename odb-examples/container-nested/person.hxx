// file      : container-nested/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <map>
#include <vector>
#include <string>
#include <utility> // std::move()
#include <cassert>

#include <odb/core.hxx>

#include <odb/nested-container.hxx> // Nested containers support.


// vector<vector<struct{...}>>
//
#pragma db value
struct country_stay
{
  std::string name;
  unsigned short days;
};

using trip = std::vector<country_stay>;
using trips = std::vector<trip>;


// vector<struct{vector<value>}>
//
using phone_numbers = std::vector<std::string>;

#pragma db value
struct emergency_contact
{
  std::string first_name;
  std::string last_name;

  #pragma db transient // Note: replaced with virtual below.
  phone_numbers numbers;
};

using emergency_contacts = std::vector<emergency_contact>;


// map<key, vector<value>>
//
using weights = std::vector<float>;
using age_weights_map = std::map<unsigned short, weights>;


#pragma db object
class person
{
public:
  person (const std::string& first, const std::string& last)
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

  // Trips.
  //
  using trips_type = ::trips;

  const trips_type&
  trips () const
  {
    return trips_;
  }

  trips_type&
  trips ()
  {
    return trips_;
  }

  // Emergency contacts.
  //
  using emergency_contacts_type = ::emergency_contacts;

  const emergency_contacts_type&
  emergency_contacts () const
  {
    return emergency_contacts_;
  }

  emergency_contacts_type&
  emergency_contacts ()
  {
    return emergency_contacts_;
  }

  // Age weights.
  //
  const age_weights_map&
  age_weights () const
  {
    return age_weights_;
  }

  age_weights_map&
  age_weights ()
  {
    return age_weights_;
  }

private:
  friend class odb::access;

  person () {}

  #pragma db id auto
  unsigned long long id_;

  std::string first_;
  std::string last_;

  // trips
  //
  // Note: commented out parts allow additional column name customizations.
  //
  #pragma db transient
  trips_type trips_;

  using _trip_key = odb::nested_key<trip>;
  using _trips_type = std::map<_trip_key, country_stay>;

  #pragma db value(_trip_key)
  //#pragma db member(_trip_key::outer) column("trip_index")
  //#pragma db member(_trip_key::inner) column("index")

  #pragma db member(trips) \
    virtual(_trips_type) \
    get(odb::nested_get (this.trips_)) \
    set(odb::nested_set (this.trips_, std::move (?))) \
    //id_column("person") key_column("") value_column("country_stay_")


  // emergency_contacts
  //
  // Note: commented out parts allow additional column name customizations.
  //
  emergency_contacts_type emergency_contacts_;

  using _phone_numbers_key = odb::nested_key<phone_numbers>;
  using _phone_numbers_type = std::map<_phone_numbers_key, std::string>;

  #pragma db value(_phone_numbers_key)
  //#pragma db member(_phone_numbers_key::outer) column("contact_index")
  //#pragma db member(_phone_numbers_key::inner) column("index")

  #pragma db member(emergency_contact_phone_numbers) \
    virtual(_phone_numbers_type) \
    after(emergency_contacts_) \
    get(odb::nested_get (this.emergency_contacts_, \
                         &emergency_contact::numbers)) \
    set(odb::nested_set (this.emergency_contacts_, \
                         &emergency_contact::numbers, \
                         std::move (?))) \
    //id_column("person") key_column("") value_column("phone_number")


  // age_weights
  //
  // Note: commented out parts allow additional column name customizations.
  //
  #pragma db transient
  age_weights_map age_weights_;

  using _weights_key = odb::nested_key<weights, unsigned short>;
  using _age_weights_type = std::map<_weights_key, float>;

  #pragma db value(_weights_key)
  //#pragma db member(_weights_key::outer) column("age")
  //#pragma db member(_weights_key::inner) column("index")

  #pragma db member(age_weights) \
    virtual(_age_weights_type) \
    get(odb::nested_get (this.age_weights_)) \
    set(odb::nested_set (this.age_weights_, std::move (?))) \
    //id_column("person") key_column("") value_column("weight")
};

#endif // PERSON_HXX
