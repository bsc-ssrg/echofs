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

#ifndef __NVM_OPEN_FILE_H__
#define __NVM_OPEN_FILE_H__

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <atomic>
#include <list>
#include <boost/filesystem.hpp>

#include "../backend.h"
#include "../../efs-common.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

const uint64_t NVML_TRANSFER_SIZE = 0x1000; // 4KiB

/* class to encapsulate a standard Unix file descriptor so that we can rely on RAII */
struct fdesc {
    int       m_fd;       /* file descriptor */
    bfs::path m_pathname; /* pathname */

    fdesc(const bfs::path& pathname, int flags=O_RDONLY);
    ~fdesc();

    size_t get_size() const;
    void close();
};

/* copy of a data block */
struct data_copy {
    off_t                m_offset;     /* file offset where the block should go */
    data_ptr_t           m_data;       /* pointer to actual data */
    std::atomic<int32_t> m_generation; /* generation number (1 == 1st copy, 2 == 2nd copy, ...) */
    std::atomic<int32_t> m_refs;       /* number of current references to this block */

    data_copy();
    data_copy(const data_copy& orig);
};

#if 0
/* descriptor of an in-NVM mmap()-ed file */
struct mapping {
    std::string          m_name;     /* mapping name */
    data_ptr_t           m_data;     /* mapped data */
    off_t                m_offset;   /* base offset within file */
    size_t               m_size;     /* mapped size */
    int                  m_is_pmem;  /* NVML-required flag */
    std::list<data_copy> m_copies;   /* block copies */

    ~mapping();
    mapping(const bfs::path& prefix, const bfs::path& base_path, size_t min_size);
    void populate(const fdesc& fd);

private:
    bfs::path generate_pool_path(const bfs::path& prefix, const bfs::path& base_path) const;
    ssize_t copy_data_to_pmem(const fdesc& fd);
    ssize_t copy_data_to_non_pmem(const fdesc& fd);
};
#endif

/* snapshot of a mapping state */
struct snapshot {
    struct mapping m_state; /* deep-copy of the mapping state when the snaspshot is created */

    snapshot(const mapping& mapping);
};

#if 0
/* descriptor for file loaded into NVML */
struct file : public Backend::file {
    /* TODO skip lists might be a good choice here.
     * e.g. see: https://github.com/khizmax/libcds 
     *      for lock-free skip lists */
    std::list<mapping>   m_mappings; /* list of mappings associated to the file */
//    std::list<data_copy> m_copies;   /* list of block copies */

    file();
    file(const mapping& mp);
    void add(const mapping& mp);
};
#endif

} // namespace nvml
} // namespace efsng

#endif /* __NVM_OPEN_FILE_H__ */
