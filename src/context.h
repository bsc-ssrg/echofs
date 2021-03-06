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

 /*
* This software was developed as part of the
* EC H2020 funded project NEXTGenIO (Project ID: 671951)
* www.nextgenio.eu
*/ 


#ifndef __EFS_CONTEXT_H__
#define __EFS_CONTEXT_H__

#include "settings.h"
#include "backends.h"
#include "api.h"
#include "thread-pool.h"

namespace efsng {

/*! Aliases for convenience */
using settings_ptr = std::unique_ptr<config::settings>;
using api_listener = api::listener<api::message<api::request, api::response>>;
using api_listener_ptr = std::unique_ptr<api_listener>;
using backend_ptr = std::unique_ptr<backend>;
using request_ptr = std::shared_ptr<api::request>;
using response_ptr = std::shared_ptr<api::response>;
using request_tracker = api::tracker<api::task_id, api::progress>;

/*! This class is used to keep the internal state of the filesystem while it's running */
struct context {

    context(const config::settings& user_opts);
    void initialize(void);
    void teardown(void);
    void trigger_shutdown(void);
    response_ptr api_handler(request_ptr request);

    settings_ptr                        m_user_args;        /*!< Configuration options passed by the user */
    api_listener_ptr                    m_api_listener;     /*!< API listener */
    std::map<std::string, backend_ptr>  m_backends;         /*!< Registered backends */
    pool                                m_thread_pool;      /*!< Thread pool for API requests */
    request_tracker                     m_tracker;          /*!< Container for tracking API requests */
    std::atomic<bool>                   m_forced_shutdown;  /*!< Flag to notify forced shutdowns */
}; // struct context

} // namespace efsng

#endif /* __EFS_CONTEXT_H__ */
