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

AM_CPPFLAGS = -I m4

TESTS = passing

check_PROGRAMS = $(TESTS)

END =

passing_CXXFLAGS = \
	-Wall -Wextra

passing_CPPFLAGS = \
	@BOOST_CPPFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/backends \
	$(END)

passing_LDFLAGS = \
	$(top_builddir)/src/libefsng.la \
	@BOOST_LDFLAGS@ \
    @BOOST_SYSTEM_LIB@ \
    @BOOST_FILESYSTEM_LIB@	\
	@LIBPMEMOBJ_LIBS@       \
	-lboost_thread \
	$(END)

passing_SOURCES = 										\
	tests-nvml-file.cpp									\
	tests-avl.cpp										\
	tests-range-lock.cpp								\
	passing-main.cpp
