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
#  Mercurium C/C++ source-to-source compiler is distributed in the hope   #
#  that it will be useful, but WITHOUT ANY WARRANTY; without even the     #
#  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR        #
#  PURPOSE.  See the GNU Lesser General Public License for more           #
#  details.                                                               #
#                                                                         #
#  You should have received a copy of the GNU Lesser General Public       #
#  License along with Mercurium C/C++ source-to-source compiler; if       #
#  not, write to the Free Software Foundation, Inc., 675 Mass Ave,        #
#  Cambridge, MA 02139, USA.                                              #
###########################################################################

ACLOCAL_AMFLAGS = -I m4

END=

bin_PROGRAMS = src/efs-ng

src_efs_ng_CXXFLAGS =       \
    $(FUSE_CFLAGS)          \
    -Wall -Wextra -std=c++11

src_efs_ng_LDFLAGS =    \
    $(FUSE_LIBS)        \
    -lboost_filesystem  \
    -lboost_system

# source files in alphabetical order
src_efs_ng_SOURCES =        \
    src/command-line.cpp    \
    src/command-line.h      \
    src/efs-ng.cpp          \
    src/metadata.cpp        \
    src/metadata.h          \
    $(END)


