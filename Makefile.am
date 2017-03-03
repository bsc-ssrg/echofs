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

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib tests

DIST_SUBDIRS = lib tests

END=

if FUSE3_BUILD
FUSEXX_CFLAGS=@FUSE3_CFLAGS@
FUSEXX_CPPFLAGS="-DFUSE_USE_VERSION=30"
FUSEXX_LIBS=@FUSE3_LIBS@
else
FUSEXX_CFLAGS=@FUSE_CFLAGS@
FUSEXX_CPPFLAGS="-DFUSE_USE_VERSION=28"
FUSEXX_LIBS=@FUSE_LIBS@
endif


bin_PROGRAMS = src/efs-ng

# we need -DBOOST_ALL_DYN_LINK to correctly link boost::log 
src_efs_ng_CPPFLAGS =		\
	$(FUSEXX_CPPFLAGS)		\
	@BOOST_CPPFLAGS@		\
	-DBOOST_ALL_DYN_LINK	\
	$(END)

src_efs_ng_CXXFLAGS =       \
    $(FUSEXX_CFLAGS)		\
    -Wall -Wextra			\
	$(END)

src_efs_ng_LDFLAGS =		\
    $(FUSEXX_LIBS)			\
    @BOOST_LDFLAGS@			\
    @BOOST_SYSTEM_LIB@		\
    @BOOST_FILESYSTEM_LIB@	\
    -lboost_log_setup       \
    @BOOST_LOG_LIB@			\
    -lboost_thread       	\
    @LIBCONFIGXX_LIBS@      \
	@LIBPMEMOBJ_LIBS@       \
    $(END)

# source files in alphabetical order
src_efs_ng_SOURCES =        				\
    src/command-line.cpp    				\
    src/command-line.h      				\
    src/configuration.cpp   				\
    src/configuration.h     				\
    src/efs-common.h                        \
    src/backends/backend.h    				\
    src/backends/backend.cpp    			\
	src/backends/dram/dram-cache.cpp		\
	src/backends/dram/dram-cache.h			\
	src/backends/nvram-nvml/nvram-cache.cpp	\
	src/backends/nvram-nvml/nvram-cache.h	\
	src/backends/nvram-nvml/nvm-open-file.h \
    src/efs-ng.cpp          				\
    src/efs-ng.h          					\
    src/logging.cpp        					\
    src/logging.h          					\
    src/metadata/dirs.cpp   				\
    src/metadata/dirs.h     				\
    src/metadata/files.cpp  				\
    src/metadata/files.h    				\
    src/preloader.cpp       				\
    src/preloader.h         				\
    src/usr-credentials.h   				\
    $(END)
