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

#include <stdarg.h>
#include <efs-api.h>

#include "messages.pb-c.h"
#include "xmalloc.h"
#include "xstring.h"
#include "requests.h"

static Efsng__Api__UserRequest* build_request_msg(request_type_t type, 
        va_list ap);
static void free_request_msg(Efsng__Api__UserRequest* reqmsg);
static Efsng__Api__UserRequest__LoadPath* build_load_path_msg(const struct efs_iocb* cbp);
static void free_load_path_msg(Efsng__Api__UserRequest__LoadPath* loadmsg);
static int remap_request_type(request_type_t type);

int
pack_to_buffer(request_type_t type, msgbuffer_t* buf, ...) {

    Efsng__Api__UserRequest* reqmsg = NULL;
    void* req_buf = NULL;
    size_t req_len = 0;
    va_list ap;
    va_start(ap, buf);

    if((reqmsg = build_request_msg(type, ap)) == NULL) {
        goto cleanup_on_error;
    }

    req_len = efsng__api__user_request__get_packed_size(reqmsg);
    req_buf = xmalloc_nz(req_len);

    if(req_buf == NULL) {
        goto cleanup_on_error;
    }

    efsng__api__user_request__pack(reqmsg, req_buf);

    buf->b_data = req_buf;
    buf->b_size = req_len;

    va_end(ap);

    return EFS_API_SUCCESS;

cleanup_on_error:
    if(reqmsg != NULL) {
        free_request_msg(reqmsg);
    }

    if(req_buf != NULL) {
        xfree(req_buf);
    }

    va_end(ap);

    return EFS_API_ENOMEM;
}

int unpack_from_buffer(msgbuffer_t* buf, response_t* resp) {

    Efsng__Api__UserResponse* respmsg;

    respmsg = efsng__api__user_response__unpack(NULL, buf->b_size, buf->b_data);

    if(respmsg == NULL) {
        return EFS_API_EUNPACKFAILED;
    }

    switch(respmsg->type) {
        case EFSNG__API__USER_RESPONSE__TYPE__ACCEPTED:
            resp->r_type = EFS_REQUEST_ACCEPTED;
            break;
        case EFSNG__API__USER_RESPONSE__TYPE__REJECTED:
            resp->r_type = EFS_REQUEST_REJECTED;
            break;
        default:
            return EFS_API_EBADRESPONSE;
    }

    if(!respmsg->has_tid && !respmsg->has_status) {
        return EFS_API_EUNPACKFAILED;
    }

    if(respmsg->has_tid) {
        resp->r_tid = respmsg->tid;
    }

    if(respmsg->has_status) {
        resp->r_status = respmsg->status;
    }

    return EFS_API_SUCCESS;
}

int remap_request_type(request_type_t type) {
    switch(type) {
        case EFS_LOAD_PATH:
            return EFSNG__API__USER_REQUEST__TYPE__LOAD_PATH;
        case EFS_UNLOAD_PATH:
            return EFSNG__API__USER_REQUEST__TYPE__UNLOAD_PATH;
        case EFS_STATUS:
            return EFSNG__API__USER_REQUEST__TYPE__STATUS;
        default:
            return -1;
    }
}

Efsng__Api__UserRequest*
build_request_msg(request_type_t type, va_list ap) {

    Efsng__Api__UserRequest* reqmsg = NULL;

    if((reqmsg = (Efsng__Api__UserRequest*) xmalloc(sizeof(*reqmsg))) == NULL) {
        goto cleanup_on_error;
    }

    efsng__api__user_request__init(reqmsg);

    switch(type) {
        case EFS_LOAD_PATH:
        {
            const struct efs_iocb* cbp = va_arg(ap, const struct efs_iocb*);

            if((reqmsg->type = remap_request_type(type)) < 0) {
                goto cleanup_on_error;
            }

            if((reqmsg->load_desc = build_load_path_msg(cbp)) == NULL) {
                goto cleanup_on_error;
            }
            break;
        }
        case EFS_UNLOAD_PATH:
        {
            const struct efs_iocb* cbp = va_arg(ap, const struct efs_iocb*);

            if((reqmsg->type = remap_request_type(type)) < 0) {
                goto cleanup_on_error;
            }

            if((reqmsg->load_desc = build_load_path_msg(cbp)) == NULL) {
                goto cleanup_on_error;
            }
            break;
        }
        case EFS_STATUS:
        {
            const struct efs_iocb* cbp = va_arg(ap, const struct efs_iocb*);

            if((reqmsg->type = remap_request_type(type)) < 0) {
                goto cleanup_on_error;
            }

            reqmsg->has_tid = true;
            reqmsg->tid = cbp->__tid;

            break;
        }
    }

    return reqmsg;

cleanup_on_error:
    free_request_msg(reqmsg);

    return NULL;
}

void
free_request_msg(Efsng__Api__UserRequest* reqmsg) {

    if(reqmsg != NULL) {
        // TODO: specific cleanup
        switch(reqmsg->type) {
            case EFSNG__API__USER_REQUEST__TYPE__LOAD_PATH:
                break;
            case EFSNG__API__USER_REQUEST__TYPE__UNLOAD_PATH:
                break;
        }
        xfree(reqmsg);
    }
}

Efsng__Api__UserRequest__LoadPath*
build_load_path_msg(const struct efs_iocb* cbp) {

    Efsng__Api__UserRequest__LoadPath* loadmsg =
        (Efsng__Api__UserRequest__LoadPath*) xmalloc(sizeof(*loadmsg));

    if(loadmsg == NULL) {
        return NULL;
    }

    efsng__api__user_request__load_path__init(loadmsg);

    loadmsg->backend = xstrdup(cbp->efs_backend);

    if(loadmsg->backend == NULL) {
        goto cleanup_on_error;
    }

    loadmsg->path = xstrdup(cbp->efs_path);

    if(loadmsg->path == NULL) {
        goto cleanup_on_error;
    }

    loadmsg->offset = cbp->efs_offset;
    loadmsg->size = cbp->efs_size;

    return loadmsg;

cleanup_on_error:
    free_load_path_msg(loadmsg);

    return NULL;
}

void
free_load_path_msg(Efsng__Api__UserRequest__LoadPath* loadmsg) {

    if(loadmsg != NULL) {
        // TODO: specific cleanup
        if(loadmsg->path != NULL) {
            xfree(loadmsg->path);
        }

        xfree(loadmsg);
    }
}
