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
#include <mutex>

#include <efs-common.h>
#include "backend-base.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace dram {

/* class to manage file allocations in DRAM */
class dram_backend : public efsng::backend {

    // some aliases for convenience
    using file_ptr = std::unique_ptr<backend::file>;

    static constexpr const char* s_name = "DRAM";

public:
    dram_backend(uint64_t capacity);
    ~dram_backend();

    std::string name() const override;
    uint64_t capacity() const override;
    error_code load(const bfs::path& pathname) override;
    error_code unload(const bfs::path& pathname) override;
    bool exists(const char* pathname) const override;

    efsng::backend::iterator find(const char* path) override;
    efsng::backend::iterator begin() override;
    efsng::backend::iterator end() override;
    efsng::backend::const_iterator cbegin() override;
    efsng::backend::const_iterator cend() override;

private:
    /* maximum allocatable size in bytes */
    uint64_t m_capacity;
    /* filename -> data */
    mutable std::mutex m_files_mutex;
    std::unordered_map<std::string, file_ptr> m_files;
}; // dram_backend

} // namespace dram
} // namespace efsng

#endif /* __DRAM_CACHE_H__ */
