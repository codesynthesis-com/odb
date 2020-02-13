// file      : common/id/auto/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <map>
#include <vector>

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_PGSQL)
#    define BLOB_TYPE "BYTEA"
#  elif defined(ODB_DATABASE_MSSQL)
#    define BLOB_TYPE "VARBINARY(max)"
#  else
#    define BLOB_TYPE "BLOB"
#  endif
#endif

#pragma db value
struct value_cont
{
  int n;
  std::vector<int> v;
};

#pragma db value
struct value_blob
{
  int n;
  #pragma db type(BLOB_TYPE)
  std::vector<char> v;
};

#pragma db object
struct object
{
  #pragma db id
  int id;

  //std::vector<std::vector<int> > vv;

  std::vector<value_cont> vc;
  std::map<int, value_cont> mc;

  std::vector<value_blob> vb;
  std::map<int, value_blob> mb;
};

/*

#include <array>
#include <vector>

using float3 = std::array<float, 3>;

#pragma db value(float3) transient
#pragma db member(float3::e1) virtual(float) get(this[0]) set(this[0])
#pragma db member(float3::e2) virtual(float) get(this[1]) set(this[1])
#pragma db member(float3::e3) virtual(float) get(this[2]) set(this[2])

#pragma db object
struct object
{
  #pragma db id
  int id;

  std::vector<float3> vf3;
};

*/
