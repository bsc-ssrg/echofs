###########################################################################
#  (C) Copyright 2016-2017 Barcelona Supercomputing Center                #
#                          Centro Nacional de Supercomputacion            #
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

SUBDIRS = lib tests doc

DIST_SUBDIRS = lib tests doc

END=

SPDLOG_INCLUDES = \
	src/spdlog/async_logger.h \
	src/spdlog/common.h \
	src/spdlog/details/async_log_helper.h \
	src/spdlog/details/async_logger_impl.h \
	src/spdlog/details/file_helper.h \
	src/spdlog/details/logger_impl.h \
	src/spdlog/details/log_msg.h \
	src/spdlog/details/mpmc_bounded_q.h \
	src/spdlog/details/null_mutex.h \
	src/spdlog/details/os.h \
	src/spdlog/details/pattern_formatter_impl.h \
	src/spdlog/details/registry.h \
	src/spdlog/details/spdlog_impl.h \
	src/spdlog/fmt/bundled/format.cc \
	src/spdlog/fmt/bundled/format.h \
	src/spdlog/fmt/bundled/ostream.cc \
	src/spdlog/fmt/bundled/ostream.h \
	src/spdlog/fmt/bundled/posix.cc \
	src/spdlog/fmt/bundled/posix.h \
	src/spdlog/fmt/bundled/time.h \
	src/spdlog/fmt/fmt.h \
	src/spdlog/fmt/ostr.h \
	src/spdlog/formatter.h \
	src/spdlog/logger.h \
	src/spdlog/sinks/android_sink.h \
	src/spdlog/sinks/ansicolor_sink.h \
	src/spdlog/sinks/base_sink.h \
	src/spdlog/sinks/dist_sink.h \
	src/spdlog/sinks/file_sinks.h \
	src/spdlog/sinks/msvc_sink.h \
	src/spdlog/sinks/null_sink.h \
	src/spdlog/sinks/ostream_sink.h \
	src/spdlog/sinks/sink.h \
	src/spdlog/sinks/stdout_sinks.h \
	src/spdlog/sinks/syslog_sink.h \
	src/spdlog/sinks/wincolor_sink.h \
	src/spdlog/spdlog.h \
	src/spdlog/tweakme.h \
	$(END)

if FUSE3_BUILD
FUSEXX_CFLAGS=@FUSE3_CFLAGS@
FUSEXX_CPPFLAGS="-DFUSE_USE_VERSION=30"
FUSEXX_LIBS=@FUSE3_LIBS@
else
FUSEXX_CFLAGS=@FUSE_CFLAGS@
FUSEXX_CPPFLAGS="-DFUSE_USE_VERSION=28"
FUSEXX_LIBS=@FUSE_LIBS@
endif

# convenience library so that we can build both the filesystem's
# binary and all unit tests
noinst_LTLIBRARIES = src/libefsng.la

# filesystem's source files in alphabetical order
src_libefsng_la_SOURCES = 							\
    src/backends/backend.h    						\
    src/backends/backend.cpp    					\
	src/backends/posix-file.cpp						\
	src/backends/posix-file.h						\
	src/backends/dram/dram.cpp						\
	src/backends/dram/dram.h						\
	src/backends/dram/file.cpp 						\
	src/backends/dram/file.h 						\
	src/backends/dram/mapping.cpp					\
	src/backends/dram/mapping.h 					\
	src/backends/dram/snapshot.cpp					\
	src/backends/dram/snapshot.h					\
	src/backends/nvram-nvml/file.cpp 				\
	src/backends/nvram-nvml/file.h 					\
	src/backends/nvram-nvml/mapping.cpp				\
	src/backends/nvram-nvml/mapping.h 				\
	src/backends/nvram-nvml/nvram-nvml.cpp			\
	src/backends/nvram-nvml/nvram-nvml.h			\
	src/backends/nvram-nvml/snapshot.cpp			\
	src/backends/nvram-nvml/snapshot.h				\
	src/range_lock.hpp								\
	src/range_lock.cpp								\
	src/avl.hpp										\
    src/command-line.cpp    						\
    src/command-line.h      						\
    src/efs-common.h                        		\
    src/logging.h          							\
    src/metadata/dirs.cpp   						\
    src/metadata/dirs.h     						\
    src/metadata/files.cpp  						\
    src/metadata/files.h    						\
    src/settings.cpp                                \
    src/settings.h                                  \
    src/usr-credentials.h   						\
    $(SPDLOG_INCLUDES)								\
    $(END)

src_libefsng_la_CPPFLAGS = 					\
	$(FUSEXX_CPPFLAGS)						\
	@BOOST_CPPFLAGS@						\
	-DBOOST_ALL_DYN_LINK					\
	-DSPDLOG_ENABLE_SYSLOG  				\
	-I$(top_srcdir)/src						\
	-I$(top_srcdir)/src/backends			\
	$(END)

src_libefsng_la_CXXFLAGS =       \
    $(FUSEXX_CFLAGS)		\
    -Wall -Wextra			\
	$(END)

# filesystem's binary
bin_PROGRAMS = src/efs-ng

src_efs_ng_SOURCES =\
    src/efs-ng.cpp          						\
    src/efs-ng.h          							\
    $(END)

EXTRA_src_efs_ng_DEPENDENCIES = \
	src/libefsng.la

# we need -DBOOST_ALL_DYN_LINK to correctly link boost::log 
src_efs_ng_CPPFLAGS =						\
	$(FUSEXX_CPPFLAGS)						\
	@BOOST_CPPFLAGS@						\
	-DBOOST_ALL_DYN_LINK					\
	-DSPDLOG_ENABLE_SYSLOG  				\
	-I$(top_srcdir)/src						\
	-I$(top_srcdir)/src/backends			\
	$(END)

src_efs_ng_CXXFLAGS =       \
    $(FUSEXX_CFLAGS)		\
    -Wall -Wextra			\
	$(END)

src_efs_ng_LDFLAGS =		\
	src/libefsng.la			\
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

