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

context::context(const config::settings& user_args) 
    : m_user_args(std::make_unique<config::settings>(user_args)),
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

    /* produce header if we are tracing */
    logger::get_global_logger()->set_pattern(TRACING_NT_PATTERN);
    LOGGER_TRACE("# trace format:\n"
                 "# timestamp:operation:pid:tid:path|backend:arguments\n"
                 "#   timestamp => epoch with usecs precision\n"
                 "#   operation => high/low level operation performed\n"
                 "#                - high level:\n"
                 "#                  access, close, closedir, create, fsync, open,\n"
                 "#                  opendir, read, readdir, stat, truncate, unlink\n"
                 "#                  write\n"
                 "#                - low level:\n"
                 "#                  nvml_read, nvml_write\n"
                 "#   pid       => PID of the process originating the call\n"
                 "#   tid       => TID of the internal thread serving the call\n"
                 "#   arguments => a list of additional fields separated by ':'\n"
                 "#                - high level operations: pathname\n"
                 "#                - read/write operations: pathname: offset, size\n"
                 "#                - nvml_read/nvml_write operations: nvml_address, size\n"
                 "#                  (note that nvml_address will be 0x0 when reading from a file gap)\n"
                 "#                  ");

    logger::get_global_logger()->flush();
    logger::get_global_logger()->set_pattern(TRACING_PATTERN);

    /* 3. Hail user */
    LOGGER_INFO("==============================================");
    LOGGER_INFO("=== echofs (NG) v{}                     ===", VERSION);
    LOGGER_INFO("==============================================");

    LOGGER_INFO("");
    LOGGER_INFO("* Deploying storage backend handlers...");

    /* 4. setup storage backends */
    for(const auto& kv : m_user_args->m_backend_opts) {

        const auto& id = kv.first;
        const auto& opts = kv.second;

        auto backend_ptr = backend::create_from_options(opts);

        LOGGER_INFO("    Backend {} (type: {})", id, backend_ptr->name());  
        LOGGER_INFO("      [ capacity: {} bytes ]", backend_ptr->capacity());

        if(backend_ptr == nullptr) {
            LOGGER_ERROR("Unable to create backend '{}'", id);
            throw std::runtime_error(""); // we don't really care about the message
        }

        m_backends.emplace(id, std::move(backend_ptr));
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
    for(const auto& kv: m_user_args->m_resources) {
        const bfs::path& pathname = kv.at("path");
        const std::string& target = kv.at("backend");
	const std::string& flag = kv.at("flags");

        if(m_backends.count(target) == 0) {
            LOGGER_WARN("Invalid backend '{}' for input resource '{}'. Ignored.", target, pathname.string());
            continue;
        }

        return_values.emplace_back(
            m_thread_pool.submit_and_track(
                    // service lambda to load files
                    [=] () -> efsng::error_code {

                        auto& backend_ptr = m_backends.at(target);
			auto m_type = backend::file::type::persistent;
			if (flag == "temporary") m_type = backend::file::type::temporary;
                        LOGGER_DEBUG("Lambda called with {} - {}", pathname, flag);

                        auto rv = backend_ptr->load(pathname, m_type);
			
                        if(rv != efsng::error_code::success) {
                            LOGGER_ERROR("Error importing {} into '{}': {}", pathname, target, rv);
                        }
                        return rv;
                    }
            )
        );
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


     std::vector<pool::task_future<efsng::error_code>> return_values;

    /* Unload any resources: TODO : We should look for persistent marked files */
/*    for(const auto& kv: m_user_args->m_resources) {     
        const bfs::path& pathname = kv.at("path");
        const std::string& target = kv.at("backend");

        if(m_backends.count(target) == 0) {
            LOGGER_WARN("Invalid backend '{}' for input resource '{}'. Ignored.", target, pathname.string());
            continue;
        }

        return_values.emplace_back(
            m_thread_pool.submit_and_track(
                    // service lambda to load files
                    [=] () -> efsng::error_code {

                        auto& backend_ptr = m_backends.at(target);

                        LOGGER_DEBUG("Lambda called with {} to {} : @ {}", pathname, target ,  m_user_args->m_results_dir);

                        auto rv = backend_ptr->unload(pathname);

                        if(rv != efsng::error_code::success) {
                            LOGGER_ERROR("Error unloading {} into '{}': {}", pathname, target, rv);
                        }

                        return rv;
                    }
            )
        );
	
    }

    // wait until all import tasks are complete before proceeding
    for(auto& rv : return_values) {
        if(rv.get() != efsng::error_code::success) {
            throw std::runtime_error("Fatal error importing resources");
        }
    }
*/
    const auto & kv = m_backends.begin();
    const auto& backend_ptr = kv->second;
 //   std::shared_ptr <efsng::backend::file> ptr;
    backend_ptr->unload(m_user_args->m_results_dir, m_user_args->m_mount_dir);
	


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

                if(m_backends.count(target) != 0) {
                    auto& backend_ptr = m_backends.at(target);
                    m_tracker.set(tid, error_code::task_in_progress);
                    auto ec = backend_ptr->load(pathname, backend::file::type::persistent);
                    m_tracker.set(tid, ec);
                }
                break;
            }
            case api::request_type::unload_path:
            {
                auto target = req->backend();
                auto pathname = req->path();

                if(m_backends.count(target) != 0) {
                    auto& backend_ptr = m_backends.at(target);
                    m_tracker.set(tid, error_code::task_in_progress);
                    auto ec = backend_ptr->unload(pathname, m_user_args->m_mount_dir);
                    m_tracker.set(tid, ec);
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
