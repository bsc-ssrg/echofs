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

#include <efs-api.h>

#define ERR_REMAP(n) ((n) < 0 ? -(n) : (n))

const char* const norns_errlist[EFS_API_ERRMAX + 1] = {
    [ERR_REMAP(EFS_API_SUCCESS)] = "Success",
    [ERR_REMAP(EFS_API_ESNAFU)] = "Internal error",
    [ERR_REMAP(EFS_API_EINVAL)] = "Invalid arguments",
    [ERR_REMAP(EFS_API_EBADREQUEST)] = "Bad request",
    [ERR_REMAP(EFS_API_EBADRESPONSE)] = "Bad response",
    [ERR_REMAP(EFS_API_ENOMEM)] = "Cannot allocate memory",
    [ERR_REMAP(EFS_API_ECONNFAILED)] = "Cannot connect to daemon",
    [ERR_REMAP(EFS_API_ERPCSENDFAILED)] = "Cannot send requests to daemon",
    [ERR_REMAP(EFS_API_ERPCRECVFAILED)] = "Cannot receive responses from daemon",
    [ERR_REMAP(EFS_API_EPACKFAILED)] = "Cannot pack request",
    [ERR_REMAP(EFS_API_EUNPACKFAILED)] = "Cannot unpack response",
    [ERR_REMAP(EFS_API_ENOSUCHTASK)] = "Task does not exist",
    [ERR_REMAP(EFS_API_ETASKPENDING)] = "Task pending",
    [ERR_REMAP(EFS_API_ETASKINPROGRESS)] = "Task in progress",
    [ERR_REMAP(EFS_API_ENOSUCHPATH)] = "Resource not found",
    [ERR_REMAP(EFS_API_EPATHEXISTS)] = "Resource already imported",

    [ERR_REMAP(EFS_API_ERRMAX)] = "Unknown error",

};

char*
efs_strerror(int errnum) {

    if(errnum > EFS_API_ERRMAX) {
        errnum = EFS_API_ERRMAX;
    }

    return (char*) norns_errlist[ERR_REMAP(errnum)];
}
