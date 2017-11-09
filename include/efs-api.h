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

#ifndef __EFS_API_H__
#define __EFS_API_H__

#include <sys/types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* echofs I/O control block */
struct efs_iocb {
    char*   efs_backend;    /* Backend prefix */
    char*   efs_path;       /* Path to resource in backend */
    off_t   efs_offset;     /* Offset */
    size_t  efs_size;       /* Size */

    /* Internal members */
    int __tid;
};

#define EFS_IOCB_INIT(backend, path, offset, size) \
{   .efs_backend = (backend), \
    .efs_path = (path), \
    .efs_offset = (offset), \
    .efs_size = (size), \
    .__tid = 0, \
}

int efs_load(struct efs_iocb* cbp);
int efs_status(struct efs_iocb* cbp);
int efs_unload(struct efs_iocb* cbp);

char* efs_strerror(int errnum);


/** Error codes */
#define EFS_API_ERRMAX 512

#define EFS_API_SUCCESS            0
#define EFS_API_ESNAFU            -1
#define EFS_API_EINVAL            -2
#define EFS_API_EBADREQUEST       -3
#define EFS_API_EBADRESPONSE      -4
#define EFS_API_ENOMEM            -5
#define EFS_API_ECONNFAILED       -6
#define EFS_API_ERPCSENDFAILED    -7
#define EFS_API_ERPCRECVFAILED    -8
#define EFS_API_EPACKFAILED       -9
#define EFS_API_EUNPACKFAILED    -10
#define EFS_API_ENOSUCHTASK      -12
#define EFS_API_ETASKPENDING     -11
#define EFS_API_ETASKINPROGRESS  -13
#define EFS_API_ENOSUCHPATH      -14
#define EFS_API_EPATHEXISTS      -15

#define EFS_API_UNKNOWN          -(EFS_API_ERRMAX)

#ifdef __cplusplus
}; // extern "C"
#endif

#endif /* __EFS_API_H__ */
