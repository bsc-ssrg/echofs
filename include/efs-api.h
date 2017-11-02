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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char*   f_backend;
    char*   f_path;
    off_t   f_offset;
    size_t  f_size;
} EFS_FILE;

#define EFS_FILE_INIT(backend, path, offset, size) \
{   .f_backend = (backend), \
    .f_path = (path), \
    .f_offset = (offset), \
    .f_size = (size) \
}

int efs_load(EFS_FILE* handle);
int efs_unload(EFS_FILE* handle);


/** Error codes */
#define EFS_API_ERRMAX 512

#define EFS_API_SUCCESS            0
#define EFS_API_ESNAFU            -1
#define EFS_API_EINVAL            -2
#define EFS_API_EBADREQUEST       -3
#define EFS_API_ENOMEM            -4
#define EFS_API_ECONNFAILED       -5
#define EFS_API_ERPCSENDFAILED    -6
#define EFS_API_ERPCRECVFAILED    -7
#define EFS_API_EPACKFAILED       -8
#define EFS_API_EUNPACKFAILED     -9
#define EFS_API_EPENDING         -10
#define EFS_API_EINPROGRESS      -11
#define EFS_API_ESUCCEEDED       -12

#ifdef __cplusplus
}; // extern "C"
#endif

#endif /* __EFS_API_H__ */
