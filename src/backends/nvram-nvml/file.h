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
#include <nvram-nvml/mapping.h>
#include <mdds/flat_segment_tree.hpp>
#include <range_lock.h>
#include <backend.h>

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

struct segment {

    segment()
        : m_data(NULL),
          m_size(0),
          m_bytes(0) { }

    segment(data_ptr_t data, size_t size, size_t bytes=0)
        : m_data(data),
          m_size(size),
          m_bytes(bytes) { }

    bool operator==(const segment& other) const {
        return m_data == other.m_data && 
                m_size == other.m_size &&
                m_bytes == other.m_bytes;
    }

    bool operator!=(const segment& other) const {
        return !(m_data == other.m_data && 
                    m_size == other.m_size &&
                    m_bytes == other.m_bytes);
    }

    data_ptr_t m_data;
    size_t m_size;
    size_t m_bytes;
};

using offset_tree_t = mdds::flat_segment_tree<off_t, mapping*>;

/* descriptor for a file loaded onto NVML */
struct file : public backend::file {

    file();
    file(const bfs::path& pathname, mapping& mp, struct stat& stbuf, file::type type=file::type::persistent);
    ~file();

    //XXX probably not needed
    void acquire_read_lock() const;
    void acquire_write_lock() const;
    void release_read_lock() const;
    void release_write_lock() const;

    lock_manager::range_lock lock_range(off_t start, off_t end, operation op) override;
    void unlock_range(lock_manager::range_lock& rl) override;

    size_t get_size() const;
    void set_size(size_t size);
    void stat(struct stat& stbuf) const;
    void save_attributes(struct stat& stbuf);

    void add(mapping&& mp);
    void extend(off_t end_offset, const std::string& prefix, const bfs::path& base_path);
    void range_lookup(off_t start, off_t end, backend::buffer_map& bmap) const;
    void find_segments(off_t start, off_t end, backend::buffer_map& bmap) const;

    bfs::path m_pathname;
    file::type m_type;
    struct stat m_attributes; /*!< File attributes */

    /* TODO skip lists might be a good choice here.
     * e.g. see: https://github.com/khizmax/libcds 
     *      for lock-free skip lists */
    std::list<mapping>  m_mappings; /* list of mappings associated to the file */

    offset_tree_t       m_offset_tree; /*!< Interval tree to speed up mapping lookups */
    mutable boost::shared_mutex m_offset_tree_mutex; /*!< Mutex to synchronize reader/writer access to the tree */

    //XXX probably not needed
    mutable boost::shared_mutex m_mutex; /*!< Mutex to synchronize reader/writer access */

    lock_manager m_lock_manager;    /*!< Manager in charge of handling range locks */
};

} // namespace nvml
} // namespace efsng

#ifdef __EFS_DEBUG__
std::ostream& operator<<(std::ostream& os, const efsng::nvml::segment& seg);
#endif /* __EFS_DEBUG__ */

#endif /* __NVML_FILE_H__ */
