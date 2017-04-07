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

#include "../backend.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace dram {

typedef void* data_ptr_t;

/* class to manage file allocations in DRAM */
class dram_backend : public efsng::Backend {

    /* a data chunk */
    struct chunk : efsng::Backend::file {
        chunk(const data_ptr_t data, const size_t size)
            : data(data),
            size(size){ }

        ~chunk(){}

        data_ptr_t  data;
        size_t      size;
    }; // struct chunk

public:
    dram_backend() : Backend(0) {} // XXX for backwards compatibility, remove

    dram_backend(int64_t size);
    ~dram_backend();

    uint64_t get_size() const;

    void prefetch(const bfs::path& pathname);
    // deprecated
    bool lookup(const char* pathname, void*& data_addr, size_t& size) const;

    bool exists(const char* pathname) const;
    void read_data(const Backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const {};
    void write_data(const Backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const {};

    efsng::Backend::iterator find(const char* path);
    efsng::Backend::iterator begin();
    efsng::Backend::iterator end();
    efsng::Backend::const_iterator cbegin();
    efsng::Backend::const_iterator cend();

private:
    /* filename -> data */
    std::unordered_map<std::string, efsng::Backend::file> entries;
}; // dram_backend

} // namespace dram
} // namespace efsng

#endif /* __DRAM_CACHE_H__ */
