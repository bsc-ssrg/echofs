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

#include <atomic>
#include <sstream>
#include <google/protobuf/stubs/common.h>
#include "api/messages.pb.h"
#include "api/requests.h"
#include "errors.h"

namespace efsng {
namespace api {

using request_ptr = std::shared_ptr<request>;
using response_ptr = std::shared_ptr<response>;

request::request(request_type type)
    : m_type(type) { }

request::request(std::string backend, const bfs::path& path, 
                 bool is_directory, off_t offset, size_t size)
    : m_type(request_type::load_path),
      m_backend(backend),
      m_path(path),
      m_is_directory(is_directory),
      m_offset(offset),
      m_size(size) { }

request::request(task_id tid)
    : m_type(request_type::status),
      m_tid(tid) { }

request_ptr request::create_from_buffer(const std::vector<uint8_t>& buffer, int size) {

    UserRequest user_req;

    if(user_req.ParseFromArray(buffer.data(), size)) {
        switch(user_req.type()) {
            case UserRequest::LOAD_PATH:

                if(user_req.has_load_desc()) {

                    auto descriptor = user_req.load_desc();

                    std::string backend = descriptor.backend();
                    bfs::path path = descriptor.path();
                    bool is_directory = descriptor.is_directory();
                    off_t offset = is_directory ? 0 : descriptor.offset();
                    size_t size = is_directory ? 0 : descriptor.size();

                    return std::make_shared<request>(backend, path, is_directory, offset, size);
                }
                break;

            case UserRequest::UNLOAD_PATH:
                break;

            case UserRequest::STATUS:

                if(user_req.has_tid()) {
                    return std::make_shared<request>(
                            static_cast<task_id>(user_req.tid())
                            );
                }
                break;

            case UserRequest::GET_CONFIG:
                break;
        }
    }

    return std::make_shared<request>(request_type::bad_request);
}

task_id request::create_tid() {
    static std::atomic<uint32_t> id(0);
    return static_cast<task_id>(++id);
}

void request::cleanup() {
    google::protobuf::ShutdownProtobufLibrary();
}

request_type request::type() const {
    return m_type;
}

std::string request::backend() const {
    return m_backend;
}

bfs::path request::path() const {
    return m_path;
}

bool request::is_directory() const {
    return m_is_directory;
}

off_t request::offset() const {
    return m_offset;
}

size_t request::size() const {
    return m_size;
}

task_id request::tid() const {
    return m_tid;
}

std::string request::to_string() const {

    std::stringstream ss;

    ss << "{";

    switch(m_type) {
        case request_type::load_path:
            ss << "LOAD ";
            ss << "b: " << m_backend << ", p: " << m_path;
            break;
        case request_type::unload_path:
            ss << "UNLOAD ";
            ss << "b: " << m_backend << ", p: " << m_path;
            break;
        case request_type::status:
            ss << "STATUS ";
            ss << "h: " << m_tid;
            break;
        case request_type::get_config:
            ss << "GET_CONFIG ";
            break;
        case request_type::bad_request:
            ss << "BAD_REQUEST";
            break;
    }

    ss << "}";

    return ss.str();
}

response::response(response_type type)
    : m_type(type),
      m_has_tid(false),
      m_tid(),
      m_has_retval(true),
      m_retval(efsng::error_code::success) { }

response::response(response_type type, efsng::error_code retval)
    : m_type(type),
      m_has_tid(false),
      m_tid(),
      m_has_retval(true),
      m_retval(retval) { }

response::response(response_type type, task_id tid, efsng::error_code retval)
    : m_type(type),
      m_has_tid(true),
      m_tid(tid),
      m_has_retval(true),
      m_retval(retval) { }

response_type response::type() const {
    return m_type;
}

task_id response::tid() const {
    return m_tid;
}

bool response::store_to_buffer(response_ptr response, std::vector<uint8_t>& buffer) {

    UserResponse user_resp;

    switch(response->type()) {
        case response_type::accepted:
            user_resp.set_type(UserResponse::ACCEPTED);
            break;
        case response_type::rejected:
            user_resp.set_type(UserResponse::REJECTED);
            break;
    }

    if(response->m_has_tid) {
        user_resp.set_tid(response->m_tid);
    }

    if(response->m_has_retval) {
        user_resp.set_status(make_api_error(response->m_retval));
    }

    size_t reserved_size = buffer.size();
    size_t message_size = user_resp.ByteSize();
    size_t buffer_size = reserved_size + message_size;

    buffer.resize(buffer_size);

    return user_resp.SerializeToArray(&buffer[reserved_size], message_size);
}

void response::cleanup() {
    google::protobuf::ShutdownProtobufLibrary();
}

} //namespace api
} // namespace efsng
