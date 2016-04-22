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

#ifndef __USR_CREDENTIALS_H__
#define __USR_CREDENTIALS_H__

#include <fuse.h>

namespace efsng{

typedef std::pair<uid_t, gid_t> Credentials;

static inline Credentials assume_user_credentials(){

    uid_t saved_uid = setfsuid(fuse_get_context()->uid);
    gid_t saved_gid = setfsuid(fuse_get_context()->gid);

    return {saved_uid, saved_gid};
}

static inline void restore_credentials(Credentials credentials){

    uid_t saved_uid = credentials.first;
    gid_t saved_gid = credentials.second;
    setfsuid(saved_uid);
    setfsuid(saved_gid);
}

} // namespace efsng

#endif /* __USR_CREDENTIALS_H__ */
