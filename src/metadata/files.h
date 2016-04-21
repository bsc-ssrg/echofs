/*************************************************************************
 * (C) Copyright 2016 Barcelona Supercomputing Center                    * 
 *                    Centro Nacional de Supercomputacion                *
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
 * Mercurium C/C++ source-to-source compiler is distributed in the hope  *
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the    *
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Mercurium C/C++ source-to-source compiler; if      *
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,       *
 * Cambridge, MA 02139, USA.                                             *
 *************************************************************************/

#ifndef __FILES_H__
#define __FILES_H__

#include <sys/types.h>

namespace efsng{

/* records metadata about an open file */
class File{

public:
    File(ino_t inode, int fd, mode_t mode);
    ~File();

    ino_t get_inode() const;
    int get_fd() const;
    mode_t get_mode() const;

private:
    /* file's inode */
    ino_t inode;
    /* file's fd */
    int fd;     
    /* file's flags at open: O_RDONLY, O_WRONLY, O_RDWR */
    mode_t mode;
};

} // namespace efsng

#endif /* __FILES_H__ */
