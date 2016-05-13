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

#ifndef __DRAM_CACHE_H__
#define  __DRAM_CACHE_H__

#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace efsng {

typedef void* data_ptr_t;

/* a data chunk */
struct chunk{
    chunk(const data_ptr_t data, const size_t size)
        : data(data),
          size(size){ }

    data_ptr_t  data;
    size_t      size;
}; // struct chunk

/* class to manage file allocations in DRAM */
class DRAM_cache{

public:
    void prefetch(const bfs::path& pathname);
    bool lookup(const char* pathname, void*& data_addr, size_t& size) const;



private:
    /* filename -> data */
    std::unordered_map<std::string, chunk> entries;
}; // DRAM_cache

} // namespace efsng

#endif /* __DRAM_CACHE_H__ */
