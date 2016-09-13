#!/bin/sh

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

# Run this to generate configure, Makefile.in's, and so on

(autoreconf --version) < /dev/null > /dev/null 2>&1 || {
  (autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo                                                                         
    echo "**Error**: You must have the GNU Build System (autoconf, automake, "   
    echo "libtool, etc) to update the efs-ng build system.  Download the "      
    echo "appropriate packages for your distribution, or get the source "        
    echo "tar balls from ftp://ftp.gnu.org/pub/gnu/."                            
    exit 1
  }

  echo                                                                           
  echo "**Error**: Your version of autoconf is too old (you need at least 2.57)" 
  echo "to update the efs-ng build system.  Download the appropriate "          
  echo "updated package for your distribution, or get the source tar ball "      
  echo "from ftp://ftp.gnu.org/pub/gnu/."                                        
  exit 1
}
