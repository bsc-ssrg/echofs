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

#include <efs-api.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "communication.h"

int efs_load(struct efs_iocb* cbp) {

    if(cbp != NULL && cbp->efs_path != NULL && cbp->efs_offset >= 0) {
        return send_load_request(cbp);
    }

    return EFS_API_EINVAL;
}

int efs_status(struct efs_iocb* cbp) {

    if(cbp != NULL) {
        return send_status_request(cbp);
    }

    return EFS_API_EINVAL;
}

int efs_unload(struct efs_iocb* cbp) {

    if(cbp != NULL) {
        return send_unload_request(cbp);
    }

    return EFS_API_EINVAL;
}
