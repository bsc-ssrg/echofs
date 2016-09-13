###########################################################################
#  (C) Copyright 2016 Barcelona Supercomputing Center                     #
#                     Centro Nacional de Supercomputacion                 #
#                                                                         #
#  This file is part of the Echo Filesystem NG.                           #
#                                                                         #
#  See AUTHORS file in the top level directory for information            #
#  regarding developers and contributors.                                 #
#                                                                         #
#  This library is free software; you can redistribute it and/or          #
#  modify it under the terms of the GNU Lesser General Public             #
#  License as published by the Free Software Foundation; either           #
#  version 3 of the License, or (at your option) any later version.       #
#                                                                         #
#  The Echo Filesystem NG is distributed in the hope that it will         #
#  be useful, but WITHOUT ANY WARRANTY; without even the implied          #
#  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR                #
#  PURPOSE.  See the GNU Lesser General Public License for more           #
#  details.                                                               #
#                                                                         #
#  You should have received a copy of the GNU Lesser General Public       #
#  License along with Echo Filesystem NG; if not, write to the Free       #
#  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     #
#                                                                         #
###########################################################################

AC_INIT([Echo Filesystem NG],
        [0.1.0],
        [alberto.miranda@bsc.es],
        [efs-ng])

AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([src/efs-ng.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])

AM_INIT_AUTOMAKE([1.9 dist-bzip2 dist-zip foreign subdir-objects])
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
LT_INIT

# check that c++11 is supported by the compiler
AX_CHECK_COMPILE_FLAG([-std=c++11], [CXXFLAGS+=" -std=c++11"], [
    echo "C++ compiler cannot compile C++11 code"
    exit -1
])

# check that FUSE >= 2.6 is available
PKG_CHECK_MODULES([FUSE], [fuse >= 2.6])

# check for several functions and libraries that affect FUSE's behavior 
AC_CHECK_FUNCS(setxattr fdatasync utimensat)
AC_CHECK_FUNCS([posix_fallocate])
AC_CHECK_LIB(ulockmgr, ulockmgr_op)
FUSE_LIBS+=$LIBS
LIBS=

# check for some BOOST libraries
AX_BOOST_BASE([1.54],, [
    AC_MSG_ERROR([
        The Echo Filesystem NG needs BOOST >= 1.54, but it was not found in your system
    ])
])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_LOG

# check for the presence of libconfig++
PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.4],,
                  AC_MSG_ERROR([libconfig++ 1.4 or newer not found.])
)

# check for the presence of the NVML libraries
PKG_CHECK_MODULES([LIBPMEM], [libpmem >= 1.1],,
                  AC_MSG_ERROR(libpmem 1.1 or newer not found.)
)

# for the POSIX compliance tests we need the tests definitions in
# the build hierarchy. This is the easiest way
AC_CONFIG_COMMANDS([posix-test-dependencies],
                   [cp -pR "$srcdir"/tests/posix-compliance/tests tests/posix-compliance/])

# likewise for consistency tests
AC_CONFIG_COMMANDS([consistency-tests-dependencies],
                   [cp -pR "$srcdir"/tests/consistency tests/])

# automatically find all read tests and put them in a EFSNG_READ_TESTS variable
# that can be easily used from Makefile.am
if test -d "$srcdir"/tests/consistency; then
    EFSNG_READ_TESTS=`cd ${srcdir}/tests/consistency/ && find read -name '*.test' -print0 | sort -z | xargs -0`
    AC_SUBST([EFSNG_READ_TESTS])
fi

AC_CONFIG_FILES([
        Makefile
        lib/Makefile
        tests/Makefile
        tests/library/Makefile
        tests/posix-compliance/Makefile
        tests/consistency/Makefile
])

AC_OUTPUT
