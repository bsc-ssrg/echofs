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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "logger.h"
#include "context.h"

namespace efsng {

context::context(const settings& user_args) 
    : m_user_args(std::make_unique<settings>(user_args)),
      m_forced_shutdown(false) { }

void context::initialize() {

    /* 1. initialize logging facilities */
    std::string log_type;
    std::string log_file;

    if(m_user_args->m_log_file != "none") {
        log_type = "file";
        log_file = m_user_args->m_log_file.string();
    }
    else {
        if(m_user_args->m_daemonize) {
            log_type = "syslog";
        }
        else{
            log_type = "console color";
        }
    }

    /* this will throw if initialization fails */
    logger::create_global_logger(m_user_args->m_exec_name, log_type, log_file);

    if(m_user_args->m_debug) {
        logger::get_global_logger()->enable_debug();
    }

    /* 2. Some useful output for debugging */
    LOGGER_DEBUG("Command line passed to FUSE:");

    std::stringstream ss;
    for(int i=0; i<m_user_args->m_fuse_argc; ++i){
        ss << m_user_args->m_fuse_argv[i] << " ";
    }

    LOGGER_DEBUG("  {}", ss.str());

    /* 3. Hail user */
    LOGGER_INFO("==============================================");
    LOGGER_INFO("=== echofs (NG) v{}                     ===", VERSION);
    LOGGER_INFO("==============================================");

    LOGGER_INFO("");
    LOGGER_INFO("* Deploying storage backend handlers...");

    /* 4. setup storage backends */
    for(const auto& kv : m_user_args->m_backend_opts){

        const auto& bend_key = kv.first;
        efsng::kv_list& bend_opts = const_cast<efsng::kv_list&>(kv.second);

        /* add root_dir as an option for those backends that may need it */
        bend_opts.push_back({"root-dir", m_user_args->m_root_dir.string()});

        efsng::backend* bend = efsng::backend::create_from_options(bend_key, bend_opts);

        LOGGER_INFO("    Backend (type: {})", bend->name());  
        LOGGER_INFO("      [ capacity: {} bytes ]", bend->capacity());

        if(bend == nullptr) {
            LOGGER_ERROR("Unable to create backend '{}'");
            throw std::runtime_error(""); // we don't really care about the message
        }

        m_backends.emplace_back(std::unique_ptr<efsng::backend>(bend));
    }

    // check that there is at least one backend
    if(m_backends.size() == 0) {
        LOGGER_ERROR("No valid backends configured. Check configuration file.");
        throw std::runtime_error(""); // we don't really care about the message
    }

    LOGGER_INFO("* Importing resources...");

    /* 5. Import any files or directories defined by the user */
    std::vector<pool::task_future<efsng::error_code>> return_values;

    /* Load any input files requested by the user to the selected backends */
    for(const auto& kv: m_user_args->m_files_to_preload){

        const bfs::path& pathname = kv.first;
        const std::string& target = kv.second;
        bool target_found = false;

        for(auto& bend : m_backends) {
            if(bend->name() == target){
                target_found = true;
                
                return_values.emplace_back(
                    m_thread_pool.submit_and_track(
                            // service lambda to load files
                            [&] () -> efsng::error_code {
                                auto rv = bend->load(pathname);

                                if(rv != efsng::error_code::success) {
                                    LOGGER_ERROR("Error importing {} into '{}': {}", pathname, target, rv);
                                }

                                return rv;
                            }
                    )
                );
                break;
            }
        }

        if(!target_found) {
            LOGGER_WARN("No configured backend '{}' for input file '{}'. Ignored.", target, pathname.string());
        }
    }

    // wait until all import tasks are complete before proceeding
    for(auto& rv : return_values) {
        if(rv.get() != efsng::error_code::success) {
            throw std::runtime_error("Fatal error importing resources");
        }
    }

    /* 6. initialize API listener */
    LOGGER_INFO("* Starting API listener...");

    if(bfs::exists(m_user_args->m_api_sockfile)) {
        bfs::remove(m_user_args->m_api_sockfile);
    }

    m_api_listener = std::make_unique<api_listener>(m_user_args->m_api_sockfile, 
            std::bind(&context::api_handler, this, std::placeholders::_1));

    m_api_listener->run();
}

void context::teardown() {

    if(m_forced_shutdown) {
        LOGGER_CRITICAL("Unrecoverable error: forcing shutdown!");
    }

    LOGGER_INFO("==============================================");
    LOGGER_INFO("=== Shutting down filesystem!!!            ===");
    LOGGER_INFO("==============================================");
    LOGGER_INFO("");

    LOGGER_INFO("* Disabling API listener...");

    if(m_api_listener != nullptr) {
        m_api_listener->stop();
    }

    if(bfs::exists(m_user_args->m_api_sockfile)) {
        bfs::remove(m_user_args->m_api_sockfile);
    }

    LOGGER_INFO("* Waiting for workers to complete...");

    m_thread_pool.stop();

    LOGGER_INFO("Bye! [status=0]");

    if(m_forced_shutdown) {
        m_forced_shutdown = false;
    }
}

/* fetch a request, unpack it and submit a lambda to process it to the thread pool */
response_ptr context::api_handler(request_ptr user_req) {

    auto req_type = user_req->type();

    if(req_type == api::request_type::bad_request) {
        return std::make_shared<api::response>(
                api::response_type::rejected,
                error_code::bad_request);
    }

    // serve those requests that were we need to give a synchronous
    // response to the user, i.e. STATUS and GET_CONFIG
    switch(req_type) {
        case api::request_type::status:
        {
            auto tid = user_req->tid();

            auto progress = m_tracker.get(tid);

            efsng::error_code status;

            if(progress) {
                status = progress->m_status;
            }
            else {
                status = error_code::no_such_task;
            }

            LOGGER_INFO("API_REQUEST: {} = {}", user_req->to_string(), status);
            return std::make_shared<api::response>(api::response_type::accepted, status);

        }

        case api::request_type::get_config:
        {
            break;
        }
        
        default:
            break;
    }

    // for asynchronous requests, generate a tid that we can return 
    // to the user so that they can later refer to them and check
    // their status
    api::task_id tid = api::request::create_tid();

    assert(!m_tracker.exists(tid));

    m_tracker.add(tid, error_code::task_pending);

    auto handler = [this] (const api::task_id tid, const request_ptr req) -> void {

        LOGGER_INFO("API_REQUEST: {}", req->to_string());

        switch(req->type()) {
            case api::request_type::load_path:
            {
                auto target = req->backend();
                auto pathname = req->path();

                for(auto& backend : m_backends) {
                    if(backend->name() == target) {
                        m_tracker.set(tid, error_code::task_in_progress);
                        auto ec = backend->load(pathname);
                        m_tracker.set(tid, ec);
                        return;
                    }
                }
                break;
            }
            case api::request_type::unload_path:
            {
                auto target = req->backend();
                auto pathname = req->path();

                for(auto& backend : m_backends) {
                    if(backend->name() == target) {
                        m_tracker.set(tid, error_code::task_in_progress);
                        auto ec = backend->unload(pathname);
                        m_tracker.set(tid, ec);
                        return;
                    }
                }
                break;
            }
            default:
                return;
        }
    };

    m_thread_pool.submit_and_forget(handler, tid, user_req);

    return std::make_shared<api::response>(api::response_type::accepted, tid, error_code::success);
}

void context::trigger_shutdown(void) {
    m_forced_shutdown = true;
    kill(getpid(), SIGTERM);
}

} // namespace efsng
