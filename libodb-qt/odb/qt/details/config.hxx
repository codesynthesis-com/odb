// file      : odb/qt/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_DETAILS_CONFIG_HXX
#define ODB_QT_DETAILS_CONFIG_HXX

// no pre

// Qt5 may complain if we are building without -fPIC. Instead of asking the
// user to pass one of these options to the ODB compiler (which can, BTW, be
// done with -x -fPIC, for example, if one is not using the Qt profile), we
// are going to define __PIC__ ourselves just to silence Qt. We also want to
// try to minimize this to cases where it is actually necessary. To achieve
// this, we need to include the Qt config file without including <QtGlobal>,
// which is where the test for PIE is. While newer versions of Qt (from 4.7)
// have <QtConfig>, to support older versions we will include qconfig.h
// directly. This file appears to be present in all the versions starting with
// Qt 4.0.
//
#ifdef ODB_COMPILER
#  if defined(__ELF__) && !defined(__PIC__)
#    include <QtCore/qconfig.h> // QT_REDUCE_RELOCATIONS
#    ifdef QT_REDUCE_RELOCATIONS
#      define __PIC__
#    endif
#  endif
#  define LIBODB_QT_STATIC_LIB
#elif !defined(LIBODB_QT_BUILD2)
#  ifdef _MSC_VER
#    include <odb/qt/details/config-vc.h>
#  else
#    include <odb/qt/details/config.h>
#  endif
#endif

// no post

#endif // ODB_QT_DETAILS_CONFIG_HXX
