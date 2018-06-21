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

#ifndef __NVML_FILE_H__
#define __NVML_FILE_H__

#include <boost/thread/shared_mutex.hpp>

#include <efs-common.h>
#include <nvram-nvml/segment.h>
#include <mdds/flat_segment_tree.hpp>
#include <range_lock.h>
#include "backend-base.h"
#include <fuse.h>
#include <atomic>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

using segment_ptr = std::shared_ptr<segment>;
using segment_tree = mdds::flat_segment_tree<off_t, segment_ptr>;
using segment_list = std::vector<segment_ptr>;

/* a contiguous file region */
struct file_region {
    file_region(data_ptr_t address, size_t size, bool is_gap, bool is_pmem)
        : m_address(address),
          m_size(size),
          m_is_gap(is_gap),
          m_is_pmem(is_pmem) { }

    data_ptr_t m_address;
    size_t m_size;
    bool m_is_gap;
    bool m_is_pmem;
};

struct file_region_list : public std::vector<file_region> {

    file_region_list() : m_size(0) {}

    // make sure that size() is not called accidentally
    size_t size() const = delete;

    size_t count() const {
        return this->std::vector<file_region>::size();
    }

    size_t total_size() const {
        return m_size;
    }

    void emplace_back(data_ptr_t address, size_t size, bool is_gap, bool is_pmem) {
        this->std::vector<file_region>::emplace_back(address, size, is_gap, is_pmem);
        m_size += size;
    }

    size_t            m_size;
};

/* descriptor for a file loaded onto NVML */
struct file : public backend::file {

    file();
    file(const bfs::path& pool_base, const bfs::path& pathname, const ino_t inode, file::type type=file::type::persistent, bool populate=true);
    ~file();
    void stat(struct stat& stbuf) const override;

    ssize_t get_data(off_t offset, size_t size, struct fuse_bufvec* fuse_buffer);
    ssize_t put_data(off_t offset, size_t size, struct fuse_bufvec* fuse_buffer);
    ssize_t append_data(off_t offset, size_t size, struct fuse_bufvec* fuse_buffer);
    void truncate(off_t offset) override;
    ssize_t allocate (off_t offset, size_t size) override;
    void save_attributes(struct stat& stbuf) override;
    int unload (const std::string dump_path) override;
    void change_type (file::type type) override;
private:

    size_t size() const;
    void update_size(size_t size);
    

    void fetch_storage(off_t offset, size_t size, file_region_list& regions);
    lock_manager::range_lock lock_range(off_t start, off_t end, operation op);
    void unlock_range(lock_manager::range_lock& rl);

    void lookup_data(off_t start, off_t end, file_region_list& regions) const;
    void lookup_segments(off_t start, off_t end, file_region_list& regions);
    void lookup_helper(off_t start, off_t end, bool alloc_gaps_as_needed, file_region_list& regions);

    void append_segments(const segment_list& segments);
    void insert_segments(const segment_list& segments);

    bfs::path generate_pool_subdir(const bfs::path& pool_base, const bfs::path& pathname) const;
    segment_ptr create_segment(off_t offset, size_t min_size, bool is_gap);

    bfs::path m_pathname;
    file::type m_type;
    bfs::path m_pool_subdir;

    struct stat m_attributes; /*!< File attributes */

    off_t m_alloc_offset; /*!< Maximum allocated offset */
    off_t m_used_offset; /*!< Maximum used offset, i.e. eof */

    uint64_t m_segment_size; /*!< Last segment size used */

    segment_tree                m_segments;
    std::atomic<bool> m_initialized; /*!< segments initialized ? */
    mutable boost::shared_mutex m_initialized_mutex;
    mutable boost::shared_mutex m_alloc_mutex; /*!< Mutex to synchronize reader/writer access to the tree */
    mutable boost::shared_mutex m_dealloc_mutex; /*!< Mutex to synchronize reader/writer access to the tree */

    mutable lock_manager m_range_mutex;    /*!< Mutex in charge of handling range locks */

};

} // namespace nvml
} // namespace efsng


#endif /* __NVML_FILE_H__ */
