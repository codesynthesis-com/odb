// file      : odb/version.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_VERSION_HXX
#define ODB_VERSION_HXX

// Version format is AABBCCDD where
//
// AA - major version number
// BB - minor version number
// CC - bugfix version number
// DD - alpha / beta (DD + 50) version number
//
// When DD is not 00, 1 is subtracted from AABBCC. For example:
//
// Version     AABBCCDD
// 2.0.0       02000000
// 2.1.0       02010000
// 2.1.1       02010100
// 2.2.0.a1    02019901
// 3.0.0.b2    02999952
//

// ODB interface version: minor, major, and alpha/beta versions.
//
#define ODB_VERSION     20476
#define ODB_VERSION_STR "2.5-b.26"

// ODB compiler version: interface version plus the bugfix version.
//
// NOTE: remember to update metadata to full version when switching to
// version.hxx.in.
//
#define ODB_COMPILER_VERSION     2049976
#define ODB_COMPILER_VERSION_STR "2.5.0-b.26"

#endif // ODB_VERSION_HXX
