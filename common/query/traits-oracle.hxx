// file      : common/query/traits-oracle.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_ORACLE_HXX
#define TRAITS_ORACLE_HXX

#include <string>
#include <memory>  // std::auto_ptr
#include <cstring> // std::memcpy
#include <cassert>

#include <odb/oracle/traits.hxx>

namespace odb
{
  namespace oracle
  {
    template <>
    class value_traits<std::auto_ptr<std::string>, id_string>
    {
    public:
      typedef std::auto_ptr<std::string> value_type;
      typedef std::string query_type;
      typedef char* image_type;

      static void
      set_value (std::auto_ptr<std::string>& v,
                 const char* b,
                 std::size_t n,
                 bool is_null)
      {
        v.reset (is_null ? 0 : new std::string (b, n));
      }

      static void
      set_image (char* b,
                 std::size_t c,
                 std::size_t& n,
                 bool& is_null,
                 const std::auto_ptr<std::string>& v)
      {
        is_null = v.get () == 0;

        if (!is_null)
        {
          n = v->size ();

          assert (c >= n);

          if (n != 0)
            std::memcpy (b, v->c_str (), n);
        }
      }
    };
  }
}

#endif // TRAITS_ORACLE_HXX
