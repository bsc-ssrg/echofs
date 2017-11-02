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

bin_PROGRAMS = \
	file_load_unload

file_load_unload_SOURCES = \
	file_load_unload.c

file_load_unload_CFLAGS = \
	-std=gnu99 -Wall -Wextra

file_load_unload_CPPFLAGS = \
	-I$(top_srcdir)/include

file_load_unload_LDADD = \
	$(top_builddir)/lib/libefs_api.la
