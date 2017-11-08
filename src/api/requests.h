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

#ifndef __API_REQUESTS_H__
#define __API_REQUESTS_H__

#include <vector>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "errors.h"
#include "utils.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace api {

using task_id = uint32_t;

enum class request_type { 
    load_path, 
    unload_path, 
    status,
    get_config,
    bad_request
};

struct request {
    using request_ptr = std::shared_ptr<request>;


    request(request_type type);
    request(std::string backend, const bfs::path& path, bool is_directory, 
            off_t offset, size_t size);
    request(task_id tid);
    ~request() = default;

    request_type type() const;
    std::string backend() const;
    bfs::path path() const;
    bool is_directory() const;
    off_t offset() const;
    size_t size() const;
    task_id tid() const;


    std::string to_string() const;

    static request_ptr create_from_buffer(const std::vector<uint8_t>& data, int size);
    static task_id create_tid();

private:
    request_type m_type;
    std::string m_backend;
    bfs::path m_path;
    bool m_is_directory;
    off_t m_offset; 
    size_t m_size;
    task_id m_tid;
};

enum class response_type { 
    accepted,
    rejected,
//    bad_request 
};

struct response {
    using response_ptr = std::shared_ptr<response>;

    response(response_type type);
    response(response_type type, efsng::error_code retval);
    response(response_type type, task_id tid, efsng::error_code retval);
    ~response() = default;
    response(const response& rhs) = delete;
    response& operator=(const response& rhs) = delete;
    response(response&& other) = default;
    response& operator=(response&& other) = default;

    response_type type() const;
    task_id tid() const;

    static bool store_to_buffer(response_ptr response, std::vector<uint8_t>& buffer);

private:
    response_type m_type;
    bool m_has_tid;
    task_id m_tid;
    bool m_has_retval;
    efsng::error_code m_retval;
};

struct progress {

    progress(efsng::error_code status) 
        : m_status(status) {}

    void update(efsng::error_code status) {
        m_status = status;
    }

    efsng::error_code m_status; /* status of the API request */
};

template <typename IdType, typename ProgressType>
struct tracker {

    tracker() = default;
    ~tracker() = default;

    template <typename... Args>
    void add(const IdType& key, Args&&... args) {

        boost::unique_lock<boost::shared_mutex> wr_lock(m_mutex);

        // duplicates not supported: if this happens, we should improve 
        // the tid generation
        assert(m_map.find(key) == m_map.end());

        m_map.emplace(key, std::forward<Args>(args)...);
    }

    boost::optional<ProgressType&> get(const IdType& key) {

        boost::shared_lock<boost::shared_mutex> rd_lock(m_mutex);

        const auto it = m_map.find(key);

        if(it != m_map.end()) {
            return boost::optional<ProgressType&>(it->second);
        }

        return boost::optional<ProgressType&>();
    }

    template <typename... Args>
    void set(const IdType& key, Args&&... args) {

        boost::unique_lock<boost::shared_mutex> wr_lock(m_mutex);

        const auto& it = m_map.find(key);

        if(it == m_map.end()) {
            return;
        }

        it->second.update(std::forward<Args>(args)...);
    }

    bool exists(const IdType& key) const {

        boost::shared_lock<boost::shared_mutex> rd_lock(m_mutex);

        return m_map.find(key) != m_map.end();
    }

private:
    mutable boost::shared_mutex m_mutex;
    std::unordered_map<IdType, ProgressType> m_map;
}; 

} //namespace api
} // namespace efsng

#endif /* __API_REQUESTS_H__ */
