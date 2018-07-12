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

 /*
* This software was developed as part of the
* EC H2020 funded project NEXTGenIO (Project ID: 671951)
* www.nextgenio.eu
*/ 


#ifndef __DIRS_H__
#define __DIRS_H__

#include <dirent.h>
#include <sys/types.h>

namespace efsng{

/* records metadata about open directory */
class Directory{

public:
    Directory(DIR* dirp, struct dirent* entry, off_t offset);
    ~Directory();

    DIR* get_dirp() const;
    struct dirent* get_entry() const;
    off_t get_offset() const;
    void set_entry(dirent* entry);
    void set_offset(off_t offset);

private:
    /* directory stream */
    DIR* dirp;
    /* buffer for next entry */
    struct dirent* entry;
    /* opaque offset (see NOTEs in readdir() man page */
    off_t offset;
};

} // namespace efsng

#endif /* __DIRS_H__ */
