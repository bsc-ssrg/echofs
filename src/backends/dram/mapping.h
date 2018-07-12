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


#ifndef __MAPPING_H__
#define __MAPPING_H__

#include <atomic>
#include <mutex>

#include <dram/file.h>
#include <posix-file.h>

namespace efsng {
namespace dram {

const uint64_t DRAM_TRANSFER_SIZE = 0x1000; // 4KiB

/* copy of a data block */
struct data_copy {
    off_t                m_offset;     /* file offset where the block should go */
    data_ptr_t           m_data;       /* pointer to actual data */
    std::atomic<int32_t> m_generation; /* generation number (1 == 1st copy, 2 == 2nd copy, ...) */
    std::atomic<int32_t> m_refs;       /* number of current references to this block */

    data_copy();
    data_copy(const data_copy& orig);
    data_copy& operator=(const data_copy& orig);
};

/* descriptor for an in-RAM mmap()-ed file region */
struct mapping {
    data_ptr_t                  m_data;     /* mapped data */
    off_t                       m_offset;   /* base offset within file */
    size_t                      m_size;     /* mapped size */
    size_t                      m_bytes;    /* used size */
    std::list<data_copy>        m_copies;   /* block copies */
    std::unique_ptr<std::mutex> m_pmutex;   /* mutex ptr (mutexes are not copyable/movable) */

    explicit mapping(const mapping& mp);

    mapping(mapping&& other) noexcept ;

    ~mapping();
    mapping(size_t min_size);
    void populate(const posix::file& fdesc);

    inline bool overlaps(off_t op_offset, size_t op_size) const {
        return ((op_offset < (off_t) (m_offset + m_size)) && 
                (m_offset <  (off_t) (op_offset + op_size)));
    }

private:
    ssize_t copy_data_to_ram(const posix::file& fdesc);
};

} // namespace dram
} // namespace efsng

#ifdef __EFS_DEBUG__

std::ostream& operator<<(std::ostream& os, const efsng::dram::mapping& mp);

#endif /* __EFS_DEBUG__ */

#endif /* __MAPPING_H__ */
