// file      : odb/oracle/oracle-fwd.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ORACLE_FWD_HXX
#define ODB_ORACLE_ORACLE_FWD_HXX

#include <odb/pre.hxx>

// Forward declaration for some of the types defined in oci.h. This
// allows us to avoid having to include oci.h in public headers.
//
typedef signed int sword;

typedef unsigned char ub1;
typedef signed char sb1;
typedef signed short sb2;
typedef unsigned short ub2;
typedef signed int sb4;
typedef unsigned int ub4;

typedef struct OCIEnv OCIEnv;
typedef struct OCISvcCtx OCISvcCtx;
typedef struct OCIError OCIError;
typedef struct OCIStmt OCIStmt;
typedef struct OCIAuthInfo OCIAuthInfo;
typedef struct OCITrans OCITrans;

typedef struct OCIParam OCIParam;
typedef struct OCILobLocator OCILobLocator;
typedef struct OCIDateTime OCIDateTime;
typedef struct OCIInterval OCIInterval;

#include <odb/post.hxx>

#endif // ODB_ORACLE_ORACLE_FWD_HXX
