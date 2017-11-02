/* * Copyright (C) 2017 Barcelona Supercomputing Center
 *                    Centro Nacional de Supercomputacion
 *
 * This file is part of the Data Scheduler, a daemon for tracking and managing
 * requests for asynchronous data transfer in a hierarchical storage environment.
 *
 * See AUTHORS file in the top level directory for information
 * regarding developers and contributors.
 *
 * The Data Scheduler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Data Scheduler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Data Scheduler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "messages.pb-c.h"

#pragma GCC visibility push(hidden)

typedef enum {
    EFS_LOAD_PATH,
    EFS_UNLOAD_PATH,
    EFS_STATUS,
    EFS_GET_CONFIG
} request_type_t;

typedef enum {
    EFS_REQUEST_ACCEPTED,
    EFS_REQUEST_REJECTED,
    EFS_BAD_REQUEST
} response_type_t;

typedef struct {
    void* b_data;
    size_t b_size;
} msgbuffer_t;

#define MSGBUFFER_INIT() \
{   .b_data = 0, \
    .b_size = 0 \
}

typedef struct {
    response_type_t r_type;
    size_t r_handle;
    int r_status;
} response_t;

#pragma GCC visibility pop

#endif /* __REQUESTS_H__ */
