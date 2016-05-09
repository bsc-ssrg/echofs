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

#include "dirs.h"

namespace efsng{

Directory::Directory(DIR* dirp, struct dirent* entry, off_t offset)
    : dirp(dirp),
      entry(entry),
      offset(offset){ }

Directory::~Directory(){}

DIR* Directory::get_dirp() const {
    return dirp;
}

struct dirent* Directory::get_entry() const{
    return entry;
}

off_t Directory::get_offset() const {
    return offset;
}

void Directory::set_entry(struct dirent* entry){
    this->entry = entry;
}

void Directory::set_offset(off_t offset){
    this->offset = offset;
}

} // namespace efsng
