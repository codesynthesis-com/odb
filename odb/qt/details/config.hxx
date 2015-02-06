// file      : odb/qt/details/config.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DETAILS_CONFIG_HXX
#define ODB_QT_DETAILS_CONFIG_HXX

// no pre

#ifdef _MSC_VER
#elif defined(ODB_COMPILER)
// Qt5 may complain if we are building without -fPIC or -fPIE. Instead of
// asking the user to pass one of these options to the ODB compiler (which
// can, BTW, be done with -x -fPIE, for example, if one is not using the
// Qt profile), we are going to define __PIE__ ourselves just to silence
// Qt. We also want to try to minimize this to cases where it is actually
// necessary. To achieve this, we need to include the Qt config file without
// including <QtGlobal>, which is where the test for PIC/PIE is. While
// newer versions of Qt (from 4.7) have <QtConfig>, to support older
// versions we will include qconfig.h directly. This file appears to be
// present in all the versions starting with Qt 4.0.
//
#  if defined(__ELF__) && !defined(__PIC__) && !defined(__PIE__)
#    include <QtCore/qconfig.h> // QT_REDUCE_RELOCATIONS
#    ifdef QT_REDUCE_RELOCATIONS
#      define __PIE__
#    endif
#  endif
#  define LIBODB_QT_STATIC_LIB
#else
#  include <odb/qt/details/config.h>
#endif

// no post

#endif // ODB_QT_DETAILS_CONFIG_HXX
