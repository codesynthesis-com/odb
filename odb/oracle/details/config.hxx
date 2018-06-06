// file      : odb/oracle/details/config.hxx
// copyright : Copyright (c) 2005-2018 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_DETAILS_CONFIG_HXX
#define ODB_ORACLE_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  error libodb-oracle header included in odb-compiled header
#elif !defined(LIBODB_ORACLE_BUILD2)
#  ifdef _MSC_VER
#    include <odb/oracle/details/config-vc.h>
#  else
#    include <odb/oracle/details/config.h>
#  endif
#endif

// no post

#endif // ODB_ORACLE_DETAILS_CONFIG_HXX
