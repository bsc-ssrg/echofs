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

if BUILD_DOCS

# directories for which doxygen should generate documentation
DX_INPUT_DIRS=				\
	$(top_srcdir)/src		\
	$(top_srcdir)/include	\
	$(END)

# input files for Sphinx
SX_INPUT_FILES=							\
	$(top_srcdir)/doc/conf.py.in 		\
	$(top_srcdir)/doc/index.rst.in 		\
	$(top_srcdir)/doc/settings.rst.in	\
	$(END)

html-local:
	@echo "Creating working directory"
	@echo "Preparing doxygen input files"
	@cat $(top_srcdir)/doc/doxygen.conf.in | \
		sed 's%INPUT = %INPUT = $(DX_INPUT_DIRS)%' > doxygen.conf
	@echo "Generating doxygen documentation"
	$(DOXYGEN) doxygen.conf
	@echo "Copying Sphinx input files"
	@for inputfile in $(SX_INPUT_FILES); \
		do \
			if test -e $${inputfile}; \
			then \
				outputfile=$${inputfile##$(top_srcdir)/doc/}; \
				outputfile=$${outputfile%%.in}; \
				outdir=`dirname $${outputfile}`; \
				test -d $${outdir} || $(MKDIR_P) $${outdir}; \
				echo "Copying '$${inputfile}' -> '$${outputfile}'"; \
				cp $${inputfile} $${outputfile}; \
			fi \
		done
	@echo "Generating Sphinx documentation"
	$(SPHINX) -c . -b html . _build

endif # BUILD_DOCS
