// file      : common/access/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <string>
#include <vector>
#include <cstring> // std::memcpy, std::memcmp, std::memset
#include <memory>  // std::auto_ptr/std::unique_ptr
#include <utility> // std::move

#include <odb/core.hxx>

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_MYSQL)
#    define BINARY16_TYPE "BINARY(16)"
#  elif defined(ODB_DATABASE_SQLITE)
#    define BINARY16_TYPE "BLOB"
#  elif defined(ODB_DATABASE_PGSQL)
#    define BINARY16_TYPE "BYTEA"
#  elif defined(ODB_DATABASE_ORACLE)
#    define BINARY16_TYPE "RAW(16)"
#  elif defined(ODB_DATABASE_MSSQL)
#    define BINARY16_TYPE "BINARY(16)"
#  elif defined(ODB_DATABASE_COMMON)
#    define BINARY16_TYPE ""
#  else
#    error unknown database
#  endif
#endif

// Test basic accessor/modifier functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object
  struct object
  {
    object (): i6_ (0), i7_ (0) {}
    object (unsigned long id, int i6, int i7): id_ (id), i6_ (i6), i7_ (i7) {}

  public:
    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id access(id)
    unsigned long id_;

  public:
    int i1 () const {return i1_;}
    int& i1 () {return i1_;}
  private:
    #pragma db access(i1)
    int i1_;

  public:
    int i2 () const {return i2_;}
    void i2 (int i2) {i2_ = i2;}
  private:
    #pragma db access(i2)
    int i2_;

    // Prefer reference modifier.
    //
  public:
    int i3 () const {return i3_;}
    int& i3 () {return i3_;}
    void i3 (int i3);
  private:
    #pragma db access(i3)
    int i3_;

    // Prefer reference modifier (reverse function order).
    //
  public:
    int i4 () const {return i4_;}
    void i4 (int i4);
    int& i4 () {return i4_;}
  private:
    #pragma db access(i4)
    int i4_;

  public:
    int get_i5 () const {return i5_;}
    void set_i5 (int i5) {i5_ = i5;}
  private:
    #pragma db get(get_i5) set(set_i5)
    int i5_;

    // Const member via reference.
    //
  public:
    const int& i6 () const {return i6_;}
  private:
    #pragma db get(i6) set(const_cast<int&> (this.i6 ()))
    const int i6_;

    // Const member via modifier.
    //
  public:
    int i7 () const {return i7_;}
    void i7 (int i7) const {const_cast<int&> (i7_) = i7;}
  private:
    #pragma db access(i7)
    const int i7_;

  public:
    const char* s1 () const {return s1_.c_str ();}
    void s1 (const char* s1) {s1_ = s1;}
    //std::string s1 () const {return s1_;}
    //void s1 (std::string s1) {s1_ = s1;}
  private:
    #pragma db get(s1) set(s1((?).c_str ()))
    //#pragma db access(s1)
    std::string s1_;

    // Array member via ref.
    //
  public:
    const char* b1 () const {return b1_;}
    char* b1 () {return b1_;}
  private:
    #pragma db type(BINARY16_TYPE) access(b1)
    char b1_[16];

    // Array member via modifier.
    //
  public:
    const char* b2 () const {return b2_;}
    void b2 (const char* b2) {std::memcpy (b2_, b2, sizeof (b2_));}
  private:
    #pragma db type(BINARY16_TYPE) access(b2)
    char b2_[16];

  public:
    bool operator== (const object& o) const
    {
      return id_ == o.id_ &&
        i1_ == o.i1_ &&
        i2_ == o.i2_ &&
        i3_ == o.i3_ &&
        i4_ == o.i4_ &&
        i5_ == o.i5_ &&
        i6_ == o.i6_ &&
        i7_ == o.i7_ &&
        s1_ == o.s1_ &&
        std::memcmp (b1_, o.b1_, sizeof (b1_)) == 0 &&
        std::memcmp (b2_, o.b2_, sizeof (b2_)) == 0;
    }
  };
}

// Test composite accessor/modifier functionality.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db value
  struct value
  {
    value () {}
    value (int i1, int i2): i1_ (i1), i2_ (i2) {}

    bool operator== (const value& v) const
    {
      return i1_ == v.i1_ && i2_ == v.i2_;
    }

  public:
    int i1 () const {return i1_;}
    int& i1 () {return i1_;}
  private:
    #pragma db access(i1)
    int i1_;

  public:
    int i2 () const {return i2_;}
    void i2 (int i2) {i2_ = i2;}
  private:
    #pragma db access(i2)
    int i2_;
  };

  #pragma db object
  struct object
  {
    object () {}
    object (unsigned long id): id_ (id) {}

    bool operator== (const object& o) const
    {
      return id_ == o.id_ &&
        v1_ == o.v1_ &&
        v2_ == o.v2_ &&
        v3_ == o.v3_;
    }

  public:
    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id access(id)
    unsigned long id_;

  public:
    const value& v1 () const {return v1_;}
    value& v1 () {return v1_;}
  private:
    #pragma db access(v1)
    value v1_;

  public:
    const value& v2 () const {return v2_;}
    void v2 (const value& v2) {v2_ = v2;}
  private:
    #pragma db access(v2)
    value v2_;

  public:
    int v3_i1 () const {return v3_.i1 ();}
    int v3_i2 () const {return v3_.i2 ();}
    void v3_i1 (int i1) {v3_.i1 () = i1;}
    void v3_i2 (int i2) {v3_.i2 (i2);}
  private:
    #pragma db get(test2::value (this.v3_i1 (), this.v3_i2 ()))    \
               set(this.v3_i1 ((?).i1 ()); this.v3_i2 ((?).i2 ()))
    value v3_;
  };
}

// Test object pointer accessor/modifier functionality.
//
#pragma db namespace table("t3_")
namespace test3
{
  struct object1;

#ifdef HAVE_CXX11
  typedef std::unique_ptr<object1> object1_ptr;
#else
  typedef std::auto_ptr<object1> object1_ptr;
#endif

  #pragma db object pointer(object1_ptr)
  struct object1
  {
    object1 () {}
    object1 (unsigned long id): id_ (id) {}

  public:
    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id access(id)
    unsigned long id_;
  };

  #pragma db object
  struct object2
  {
    object2 () {}
    object2 (unsigned long id): id_ (id) {}

  public:
    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id access(id)
    unsigned long id_;

  public:
    const object1_ptr& p1 () const {return p1_;}
    object1_ptr& p1 () {return p1_;}
  private:
    #pragma db access(p1)
    object1_ptr p1_;

  public:
    const object1_ptr& p2 () const {return p2_;}

#ifdef HAVE_CXX11
    void p2 (object1_ptr p2) {p2_ = std::move (p2);}
#else
    void p2 (object1_ptr p2) {p2_ = p2;}
#endif
  private:
#ifdef HAVE_CXX11
    #pragma db get(p2) set(p2 (std::move (?)))
#else
    #pragma db access(p2)
#endif
    object1_ptr p2_;
  };
}

// Test container accessor/modifier functionality.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db value
  struct value
  {
    value (): c3_ (3, 999) {}
    value (int v): c1_ (3, v), c2_ (3, v + 1), c3_ (3, v + 2) {}

    bool operator== (const value& v) const
    {
      return c1_ == v.c1_ && c2_ == v.c2_ && c3_ == v.c3_;
    }

  public:
    const std::vector<int>& c1 () const {return c1_;}
    std::vector<int>& c1 () {return c1_;}
  private:
    #pragma db access(c1)
    std::vector<int> c1_;

  public:
    const std::vector<int>& c2 () const {return c2_;}
    void c2 (const std::vector<int>& c2) {c2_ = c2;}
  private:
    #pragma db access(c2)
    std::vector<int> c2_;

  public:
    const std::vector<int> c3_;
  };

  #pragma db object
  struct object
  {
    object () {}
    object (unsigned long id): id_ (id), c3_ (3, 3123), v2_ (2123) {}

    bool operator== (const object& o) const
    {
      return id_ == o.id_ &&
        c1_ == o.c1_ &&
        c2_ == o.c2_ &&
        c3_ == o.c3_ &&
        v1_ == o.v1_ &&
        v2_ == o.v2_;
    }

  public:
    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id access(id)
    unsigned long id_;

  public:
    const std::vector<int>& c1 () const {return c1_;}
    std::vector<int>& c1 () {return c1_;}
  private:
    #pragma db access(c1)
    std::vector<int> c1_;

  public:
    const std::vector<int>& c2 () const {return c2_;}
    void c2 (const std::vector<int>& c2) {c2_ = c2;}
  private:
    #pragma db access(c2)
    std::vector<int> c2_;

  public:
    const std::vector<int>& c3 () const {return c3_;}
  private:
    #pragma db get(c3) set(const_cast<std::vector<int>&> (this.c3 ()))
    const std::vector<int> c3_;

  public:
    const value& v1 () const {return v1_;}
    value& v1 () {return v1_;}
  private:
    #pragma db access(v1)
    value v1_;

  public:
    const value v2_;
  };
}

// Test id accessor/modifier functionality.
//
#pragma db namespace table("t5_")
namespace test5
{
  #pragma db object
  struct object1
  {
    object1 (): id_ (0) {}

  public:
    unsigned long id () const {return id_;}
    void id (unsigned long id) {id_ = id;}
  private:
    #pragma db id auto access(id)
    unsigned long id_;
  };

  #pragma db object
  struct object2
  {
    object2 (): id_ (0) {}

  public:
    unsigned long id () const {return id_;}
    unsigned long& id () {return id_;}
  private:
    #pragma db id auto access(id)
    unsigned long id_;
  };

  #pragma db object
  struct object3
  {
    object3 (): id_ (0) {}

    #pragma db id auto
    const unsigned long id_;
  };

  #pragma db value
  struct uuid
  {
    uuid () {std::memset (data_, 0, sizeof (data_));}
    explicit uuid (const char* d) {data (d);}

  public:
    const char* data () const {return data_;}
    void data (const char* d) {std::memcpy (data_, d, sizeof (data_));}
  private:
    #pragma db type(BINARY16_TYPE) column("") access(data)
    char data_[16];
  };

  #pragma db object
  struct object4
  {
  public:
    const uuid& id () const {return id_;}
    void id (const uuid& id) {id_ = id;}
  private:
    #pragma db id access(id)
    uuid id_;
  };
}

// Test version accessor/modifier functionality.
//
#pragma db namespace table("t6_")
namespace test6
{
  #pragma db object optimistic
  struct object1
  {
    object1 (unsigned long id = 0): id_ (id), version_ (0) {}

    #pragma db id
    unsigned long id_;

  public:
    unsigned long version () const {return version_;}
    void version (unsigned long version) {version_ = version;}
  private:
    #pragma db version access(version)
    unsigned long version_;
  };

  #pragma db object optimistic
  struct object2
  {
    object2 (): version_ (0) {}

    #pragma db id auto
    unsigned long id_;

  public:
    unsigned long version () const {return version_;}
    unsigned long& version () {return version_;}
  private:
    #pragma db version access(version)
    unsigned long version_;
  };

  #pragma db object optimistic
  struct object3
  {
    object3 (unsigned long id = 0): id_ (id), version_ (0) {}

    #pragma db id
    unsigned long id_;

    #pragma db version
    const unsigned long version_;
  };
}

// Test automatic discovery of accessor/modifier functions.
//
#pragma db namespace table("t7_")
namespace test7
{
  #pragma db object
  struct object
  {
    object () {}
    object (unsigned long id): id_ (id) {}

    #pragma db id
    unsigned long id_;

  public:
    int i1 () const {return i1_;}
    int& i1 () {return i1_;}
  private:
    int i1_;

  public:
    const int& get_i2 () const {return i2_;}
    void set_i2 (int i2) {i2_ = i2;}
  private:
    int i2_;

  public:
    const int& getI3 () const {return i3_;}
    void setI3 (const int& i3) {i3_ = i3;}
  private:
    int i3_;

  public:
    int geti4 () const {return i4;}
    int seti4 (int v) {int r (i4); i4 = v; return r;}
  private:
    int i4;

    // Prefer reference modifier.
    //
  public:
    int i5 () const {return i5_;}
    int& i5 () {return i5_;}
    void i5 (int i5);
  private:
    int i5_;

    // Prefer reference modifier (reverse function order).
    //
  public:
    int i6 () const {return i6_;}
    void i6 (int i6);
    int& i6 () {return i6_;}
  private:
    int i6_;

    // Custom accessor/modifier regex.
    //
  public:
    int GetI7 () const {return i7_;}
    void SetI7 (int i7) {i7_ = i7;}
  private:
    int i7_;

    // Array member via ref.
    //
  public:
    const char* b1 () const {return b1_;}
    char* b1 () {return b1_;}
  private:
    #pragma db type(BINARY16_TYPE)
    char b1_[16];

    // Array member via modifier.
    //
  public:
    const char* b2 () const {return b2_;}
    void b2 (const char* b2) {std::memcpy (b2_, b2, sizeof (b2_));}
  private:
    #pragma db type(BINARY16_TYPE)
    char b2_[16];

  public:
    bool operator== (const object& o) const
    {
      return id_ == o.id_ &&
        i1_ == o.i1_ &&
        i2_ == o.i2_ &&
        i3_ == o.i3_ &&
        i4  == o.i4  &&
        i5_ == o.i5_ &&
        i6_ == o.i6_ &&
        i7_ == o.i7_ &&
        std::memcmp (b1_, o.b1_, sizeof (b1_)) == 0 &&
        std::memcmp (b2_, o.b2_, sizeof (b2_)) == 0;
    }
  };
}

#endif // TEST_HXX
