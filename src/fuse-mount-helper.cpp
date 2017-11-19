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

#include <fuse.h>
#include <stdlib.h>
#include "utils.h"
#include "context.h"
#include "fuse-mount-helper.h"

namespace efsng {

int fuse_custom_mounter(const config::settings& user_opts, const struct fuse_operations* ops) {
    
	struct fuse *fuse;
	char *mountpoint;
	int multithreaded;
	int res;

    /* create a context object to maintain the internal state of the filesystem 
     * so that we can later pass it around to the filesystem operations */
    auto efsng_ctx = std::make_unique<efsng::context>(user_opts);

	fuse = fuse_setup(user_opts.m_fuse_argc, 
	                  const_cast<char**>(user_opts.m_fuse_argv), 
	                  ops, 
	                  sizeof(*ops), 
	                  &mountpoint,
				      &multithreaded, 
				      (void*) efsng_ctx.get());

	if(fuse == NULL) {
		return 1;
    }

	if(multithreaded) {
		res = fuse_loop_mt(fuse);
    }
	else {
		res = fuse_loop(fuse);
    }

	fuse_teardown(fuse, mountpoint);

	if (res == -1) {
		return 1;
	}

	return 0;
}

} //namespace efsng
