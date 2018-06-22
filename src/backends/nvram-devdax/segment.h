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

#ifndef __SEGMENT_H__
#define __SEGMENT_H__

#include <atomic>
#include <mutex>

#include <nvram-devdax/file.h>
#include <posix-file.h>
#include <bitset>

namespace efsng {
namespace nvml_dev {

static const uint64_t NVML_TRANSFER_SIZE = 0x1000; // 4KiB
static const uint64_t NVML_MAPPINGS = 0x1000000; //16MB of mappings

struct big_pool {
    big_pool() {m_init = false;}
    int init(const bfs::path& mmap, size_t length);
    ~big_pool();
    std::bitset<NVML_MAPPINGS> m_bitset;
    size_t m_offset; /*!< Last offset used */
    bfs::path  m_path;
    int m_fd; /*!< File descriptor */
    void * m_address; /*!< Starting address for the mmap */
    bool m_init;
    size_t m_length; /*!< Specified length (via conf) */
    size_t m_size_of_segment; /*!< Size of a segment */
    size_t m_NVML_MAPPINGS;
    void * allocate(size_t size); /*!< Allocates n segment */
    void deallocate(void * address, size_t length); /*!< De-allocates n segment */

    std::mutex m_allocate_mutex;
};

struct pool {
    pool(const bfs::path& subdir);
    ~pool();
    void allocate(size_t size);
    void deallocate();
    static big_pool  m_bpool; /*! < Pool storing the big mmap file */
    bfs::path                   m_subdir;   /*!< Subdir to store file segments */
    bfs::path                   m_path;     /*!< Segment's 'filesystem name' */
    data_ptr_t                  m_data;     /*!< Mapped data */
    size_t                      m_length;
    int                         m_is_pmem;  /*!< NVML-required flag */
};

/* descriptor for an in-NVM mmap()-ed file region */
struct segment {

    #ifdef __ECHOFS_DAXFS
    static const size_t s_min_size = 0x100000;
    static const size_t s_segment_size = 0x100000*1; //0x100000*1; // 1MiB
    #else
    static const size_t s_min_size = 0x100000;//0x100000*128;
    static const size_t s_segment_size = 0x100000;//0x100000*128; //0x100000;
    #endif

    off_t                       m_offset;   /*!< Base offset within file */
    size_t                      m_size;     /*!< Mapped size */
    bool                        m_is_gap;   /*!< Segment is a zero-filled gap */
    struct pool                 m_pool;     /*!< Pool descriptor for the segment */
    bool                        m_is_free;  /*!< Mark as free to reuseit */
    size_t                      m_bytes;    /*!< Used size */ /* TODO : Reducir para el truncate */

    segment(const bfs::path& subdir, off_t offset, size_t size, bool is_gap);
    ~segment();

    static void sync_all();

    void allocate(off_t offset, size_t size);
    void deallocate();
    bool is_pmem() const;
    data_ptr_t data() const;

    size_t fill_from(const posix::file& fdesc);

    inline bool overlaps(off_t op_offset, size_t op_size) const {
        return ((op_offset < (off_t) (m_offset + m_size)) && 
                (m_offset <  (off_t) (op_offset + op_size)));
    }

    void zero_fill(off_t offset, size_t size);

private:
    ssize_t copy_data_to_pmem(const posix::file& fdesc);
    ssize_t copy_data_to_non_pmem(const posix::file& fdesc);
};

} // namespace nvml
} // namespace efsng

#ifdef __EFS_DEBUG__

std::ostream& operator<<(std::ostream& os, const efsng::nvml::segment& mp);

#endif /* __EFS_DEBUG__ */

#endif /* __SEGMENT_H__ */
