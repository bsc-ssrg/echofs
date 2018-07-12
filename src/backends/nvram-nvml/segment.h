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


#ifndef __SEGMENT_H__
#define __SEGMENT_H__

#include <atomic>
#include <mutex>

#include <nvram-nvml/file.h>
#include <posix-file.h>

namespace efsng {
namespace nvml {

static const uint64_t NVML_TRANSFER_SIZE = 0x1000; // 4KiB

struct pool {
    pool(const bfs::path& subdir);
    ~pool();
    void allocate(size_t size);

    bfs::path                   m_subdir;   /*!< Subdir to store file segments */
    bfs::path                   m_path;     /*!< Segment's 'filesystem name' */
    data_ptr_t                  m_data;     /*!< Mapped data */
    size_t                      m_length;
    int                         m_is_pmem;  /*!< NVML-required flag */
};

/* descriptor for an in-NVM mmap()-ed file region */
struct segment {

    constexpr static const size_t default_segment_size = 128*1024*1024;
    static size_t s_segment_size; // = 512*1024*1024; // 512MiB


    off_t                       m_offset;   /*!< Base offset within file */
    size_t                      m_size;     /*!< Mapped size */
    bool                        m_is_gap;   /*!< Segment is a zero-filled gap */
    struct pool                 m_pool;     /*!< Pool descriptor for the segment */

    size_t                      m_bytes;    /*!< Used size */ /* TODO : Reducir para el truncate */

    segment(const bfs::path& subdir, off_t offset, size_t size, bool is_gap);
    ~segment();

    static void sync_all();

    void allocate(off_t offset, size_t size);
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
