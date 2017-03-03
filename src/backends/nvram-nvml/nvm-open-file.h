/*************************************************************************
 * (C) Copyright 2016-2017 Barcelona Supercomputing Center               *
 *                         Centro Nacional de Supercomputacion           *
 *                                                                       *
 * This file is part of the Echo Filesystem NG.                          *
 *                                                                       *
 * See AUTHORS file in the top level directory for information           *
 * regarding developers and contributors.                                *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public            *
 * License as published by the Free Software Foundation; either          *
 * version 3 of the License, or (at your option) any later version.      *
 *                                                                       *
 * The Echo Filesystem NG is distributed in the hope that it will        *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied         *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR               *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Echo Filesystem NG; if not, write to the Free      *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    *
 *                                                                       *
 *************************************************************************/

#ifndef __NVM_OPEN_FILE_H__
#define __NVM_OPEN_FILE_H__

#include "../../efs-common.h"

/* descriptor of an in-NVM mmap()-ed file */
struct nvm_file_map {
    data_ptr_t data; /* mapped data */
    size_t     size; /* mapped size */
};

/* copy of a data block */
struct nvm_copy {
    int        generation; /* generation number (1 == 1st copy, 2 == 2nd copy, ...) */
    off_t      offset;     /* file offset where the block should go */
    data_ptr_t data;       /* pointer to actual data */
    int        refs;       /* number of current references to this block */
};

/* descriptor for an nvm open file */
struct nvm_open_file {
    /* TODO skip lists might be a good choice here.
     * e.g. see: https://github.com/khizmax/libcds 
     *      for lock-free skip lists */
    std::vector<nvm_file_map> nvm_maps;   /* list of mmap pools associated to the file */
    std::vector<nvm_copy>     nvm_copies; /* list of copies */
};

#endif /* __NVM_OPEN_FILE_H__ */
