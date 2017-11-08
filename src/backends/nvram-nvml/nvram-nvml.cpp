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

/* C includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <libpmem.h>

/* C++ includes */
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <memory>
#include <chrono>
#include <thread>

/* internal includes */
#include <logging.h>
#include <utils.h>
#include <posix-file.h>
#include <nvram-nvml/file.h>
#include <nvram-nvml/segment.h>
#include <nvram-nvml/nvram-nvml.h>

namespace efsng {
namespace nvml {

nvml_backend::nvml_backend(uint64_t capacity, bfs::path daxfs_mount, bfs::path root_dir, logger& logger)
    : m_capacity(capacity),
      m_logger(logger),
      m_daxfs_mount_point(daxfs_mount),
      m_root_dir(root_dir) {
}

nvml_backend::~nvml_backend(){
}

std::string nvml_backend::name() const {
    return s_name;
}

uint64_t nvml_backend::capacity() const {
    return m_capacity;
}

/** start the load process of a file requested by the user */
/* pathname = file path in underlying filesystem */
error_code nvml_backend::load(const bfs::path& pathname) {

#ifdef __EFS_DEBUG__
    m_logger.debug("Import {} to NVRAM", pathname);
#endif

    if(!bfs::exists(pathname)) {
        return error_code::no_such_path;
    }

    /* add the mapping to a nvml::file descriptor and insert it into m_files */
    std::lock_guard<std::mutex> lock(m_files_mutex);

    /* create a new file into m_files (the constructor will fill it with
     * the contents of the pathname) */
    auto it = m_files.emplace(pathname.c_str(), 
                              std::make_unique<nvml::file>(m_daxfs_mount_point, pathname));

#ifdef __EFS_DEBUG__
    const auto& file_ptr = (*(it.first)).second;

    struct stat stbuf;
    file_ptr->stat(stbuf);
    m_logger.debug("Transfer complete ({} bytes)", stbuf.st_size);
#endif

    return error_code::success;

    /* lock_guard is automatically released here */
}

error_code nvml_backend::unload(const bfs::path& pathname) {
    m_logger.error("Unload not implemented!");
    return error_code::success;
}

bool nvml_backend::exists(const char* pathname) const {

    std::lock_guard<std::mutex> lock(m_files_mutex);

    const auto& it = m_files.find(pathname);

    // XXX it is safe to unlock the mutex here BECAUSE WE KNOW
    // that no other threads are going to remove a mapping from
    // the list; this happens because mappings are ONLY destroyed
    // when the std::list destructor is called (and we make sure that
    // this is done in a thread-safe manner). If this assumption ever
    // changes, we may NEED to revise this.
    return it != m_files.end();
}

backend::iterator nvml_backend::find(const char* path) {
    return m_files.find(path);
}

backend::iterator nvml_backend::begin() {
    return m_files.begin();
}

backend::iterator nvml_backend::end() {
    return m_files.end();
}

backend::const_iterator nvml_backend::cbegin() {
    return m_files.cbegin();
}

backend::const_iterator nvml_backend::cend() {
    return m_files.cend();
}

} // namespace nvml
} //namespace efsng
