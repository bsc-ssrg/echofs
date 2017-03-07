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

#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>

#include "../backend.h"
#include "nvm-open-file.h"

namespace bfs = boost::filesystem;

namespace efsng {

typedef void* data_ptr_t;

/* class to manage file allocations in NVRAM */
class NVRAM_cache : public Backend {

    const uint64_t block_size = 4096;

    /* a data chunk */
    struct chunk{
        chunk(const data_ptr_t data, const size_t size)
            : data(data),
              size(size){ }

        data_ptr_t  data;
        size_t      size;
    }; // struct chunk

public:
    NVRAM_cache() : Backend(0) {} // XXX for backwards compatibility, remove

    NVRAM_cache(int64_t size, bfs::path dax_fs_base, bfs::path root_dir);
    ~NVRAM_cache();

    uint64_t get_size() const;

    void prefetch(const bfs::path& pathname);
    bool lookup(const char* pathname, void*& data_addr, size_t& size) const;

private:
    ssize_t do_copy_to_pmem(char*, int);
    ssize_t do_copy_to_non_pmem(char*, int);

private:
    /* mount point of the DAX filesystem needed to access NVRAM */
    bfs::path dax_fs_base;
    bfs::path root_dir;
    /* filename -> data */
    std::unordered_map<std::string, chunk> entries;
}; // NVRAM_cache

} // namespace efsng

#endif /* __NVRAM_CACHE_H__ */
