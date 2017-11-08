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
#include "errors.h"

namespace efsng {

int make_api_error(error_code ec) {

    switch(ec) {
        case error_code::success:
            return EFS_API_SUCCESS;
        case error_code::internal_error:
             return EFS_API_ESNAFU;
        case error_code::invalid_arguments:
             return EFS_API_EINVAL;
        case error_code::bad_request:
            return EFS_API_EBADREQUEST;
        case error_code::no_such_task:
            return EFS_API_ENOSUCHTASK;
        case error_code::task_pending:
            return EFS_API_ETASKPENDING;
        case error_code::task_in_progress:
            return EFS_API_ETASKINPROGRESS;
        case error_code::no_such_path:
            return EFS_API_ENOSUCHPATH;
        default:
            return EFS_API_UNKNOWN;
    }

    return 0;
}

} // namespace efsng

std::ostream& operator << (std::ostream& os, const efsng::error_code& ec) {

    switch(ec) {

        case efsng::error_code::success:
            os << "success";
            break;
        case efsng::error_code::internal_error:
            os << "internal error";
            break;
        case efsng::error_code::invalid_arguments:
            os << "invalid arguments";
            break;
        case efsng::error_code::bad_request:
            os << "bad request";
            break;
        case efsng::error_code::no_such_task:
            os << "no such task";
            break;
        case efsng::error_code::task_pending:
            os << "task pending";
            break;
        case efsng::error_code::task_in_progress:
            os << "task in progress";
            break;
        case efsng::error_code::no_such_path:
            os << "path to resource not found";
            break;
    }

    return os;
}
