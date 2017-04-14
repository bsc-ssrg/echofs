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

#ifndef __NVRAM_CACHE_H__
#define  __NVRAM_CACHE_H__

/* C++ includes */
#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include <mutex>

/* internal includes */
#include <efs-common.h>
#include <logging.h>
#include <backend.h>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

/* class to manage file allocations in NVRAM based on the NVML library */
class nvml_backend : public efsng::backend {

    static constexpr const char* s_name = "NVRAM-NVML";

public:
    nvml_backend(uint64_t capacity, bfs::path daxfs_mount, bfs::path root_dir, logger& logger);
    ~nvml_backend();

    std::string name() const override;
    uint64_t capacity() const override;
    void load(const bfs::path& pathname) override;
    bool exists(const char* pathname) const override;
    void read_data(const backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const override;
    void write_data(const backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const override;

    backend::iterator find(const char* path) override;
    backend::iterator begin() override;
    backend::iterator end() override;
    backend::const_iterator cbegin() override;
    backend::const_iterator cend() override;

private:
    /* maximum allocatable size in bytes */
    uint64_t m_capacity;
    logger& m_logger;
    /* mount point of the DAX filesystem needed to access NVRAM */
    bfs::path m_daxfs_mount_point;
    bfs::path m_root_dir;

    mutable std::mutex                    m_files_mutex;
    std::unordered_map<std::string, 
                       std::unique_ptr<backend::file>> m_files;
}; // nvml_backend

} // namespace nvml
} // namespace efsng

#endif /* __NVRAM_CACHE_H__ */
