// file      : common/driver/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <string>
#include <memory> // std::auto_ptr, std::unique_ptr

#include <odb/core.hxx>

// Test basic functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object bulk(3) session
  struct object
  {
    object (unsigned int n_ = 0, std::string s_ = "")
        : id (0), n (n_), s (s_) {}

    #pragma db id auto
    unsigned long id;

    unsigned int n;

    #pragma db oracle:type("CLOB") mssql:type("VARCHAR(max)") // Long data.
    std::string s;
  };

  #pragma db object bulk(3) pointer(std::auto_ptr)
  struct auto_object
  {
    auto_object (unsigned int n_ = 0, std::string s_ = "")
        : id (0), n (n_), s (s_) {}

    #pragma db id auto
    unsigned long id;

    unsigned int n;
    std::string s;
  };

#ifdef HAVE_CXX11
  #pragma db object bulk(3) pointer(std::unique_ptr)
  struct unique_object
  {
    unique_object (unsigned int n_ = 0, std::string s_ = "")
        : id (0), n (n_), s (s_) {}

    #pragma db id auto
    unsigned long id;

    unsigned int n;
    std::string s;
  };
#endif
}

// Test object with manually assigned id.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db object bulk(3) session
  struct object
  {
    // Can't use empty id because of Oracle.
    //
    object (std::string id_ = "!", unsigned int n_ = 0, std::string s_ = "")
        : id (id_), n (n_), s (s_) {}

    #pragma db id
    std::string id;

    unsigned int n;
    std::string s;
  };

#ifdef HAVE_CXX11
#pragma db object bulk(3) pointer(std::unique_ptr)
  struct unique_object
  {
    unique_object (std::string id_ = "",
                   unsigned int n_ = 0,
                   std::string s_ = "")
        : id (id_), n (n_), s (s_) {}

    #pragma db id
    std::string id;

    unsigned int n;
    std::string s;
  };
#endif
}

// Test failure.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db object bulk(3)
  struct object
  {
    object (unsigned long id_ = 0, unsigned int n_ = 0)
        : id (id_), n (n_), s ("abc") {}

    #pragma db id
    unsigned long id;

    #pragma db unique
    unsigned int n;
    std::string s;
  };
}

// Test a large batch.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object bulk(3000)
  struct object
  {
    object (unsigned int n_ = 0, std::string s_ = "")
        : id (0), n (n_), s (s_) {}

    #pragma db id auto
    unsigned long id;

    unsigned int n;

    #pragma db oracle:type("CLOB") mssql:type("VARCHAR(max)") // Long data.
    std::string s;
  };
}

// Test object without id.
//
#pragma db namespace table("t5_")
namespace test5
{
  #pragma db object no_id bulk(3)
  struct object
  {
    object (unsigned int n_ = 0, std::string s_ = ""): n (n_), s (s_) {}

    unsigned int n;
    std::string s;
  };
}

// Test API with persistent class template instantiations.
//
#pragma db namespace table("t6_")
namespace test6
{
  template <int>
  struct object_template
  {
    object_template (unsigned int n_ = 0, std::string s_ = "")
        : id (0), n (n_), s (s_) {}

    unsigned long id;
    unsigned int n;
    std::string s;
  };

  typedef object_template<1> object;

  #pragma db object(object) bulk(3)
  #pragma db member(object::id) id auto

  typedef object_template<2> auto_object;

  #pragma db object(auto_object) bulk(3) pointer(std::auto_ptr)
  #pragma db member(auto_object::id) id auto

#ifdef HAVE_CXX11
  typedef object_template<3> unique_object;

  #pragma db object(unique_object) bulk(3) pointer(std::unique_ptr)
  #pragma db member(unique_object::id) id auto
#endif
}

// Test optimistic concurrency.
//
#pragma db namespace table("t7_")
namespace test7
{
  #pragma db object optimistic bulk(3)
  struct object
  {
    object (unsigned int n_ = 0, std::string s_ = "")
        : id (0), v (0), n (n_), s (s_) {}

    #pragma db id auto
    unsigned long long id;

    #pragma db version
    unsigned long long v;

    unsigned int n;
    std::string s;
  };
}

// Test SQL Server optimistic concurrency with ROWVERSION.
//
#if defined(ODB_DATABASE_MSSQL) || defined(DATABASE_MSSQL)
#pragma db namespace table("t8_")
namespace test8
{
  #pragma db object optimistic bulk(3)
  struct object
  {
    object (unsigned int n_ = 0, std::string s_ = "")
        : id (0), v (0), n (n_), s (s_) {}

    #pragma db id
    unsigned long long id;

    #pragma db version type("ROWVERSION")
    unsigned long long v;

    unsigned int n;
    std::string s;
  };
}
#endif

#endif // TEST_HXX
