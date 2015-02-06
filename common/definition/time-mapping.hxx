// file      : common/definition/time-mapping.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TIME_MAPPING_HXX
#define TIME_MAPPING_HXX

#ifdef _WIN32
#  include <winsock2.h> // timeval
#else
#  include <sys/time.h> // timeval
#endif

#pragma db value(timeval) definition
#pragma db member(timeval::tv_sec) column("sec")
#pragma db member(timeval::tv_usec) column("usec")

#endif // TIME_MAPPING_HXX
