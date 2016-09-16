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

#ifndef __DATA_STORE_H__
#define __DATA_STORE_H__

#include <cstdint>
#include <string>

#include "../command-line.h"

namespace efsng {

/**
 * Backend: A pure virtual class for manipulating a backend store
 */
class Backend {

public:
    enum Type {
        UNKNOWN = -1,
        DRAM = 0,
        NVRAM_NVML,

        /* add newly registered backend types ABOVE this line */
        TOTAL_COUNT
    };

protected:
    Backend(int64_t size)
        : max_size(size) {}
    virtual ~Backend() {}

public:
    static Type name_to_type(const std::string& name);
    static Backend* backend_factory(const std::string& type, const kv_list& backend_opts);
    virtual uint64_t get_size() const = 0;
    virtual void prefetch(const bfs::path& pathname) = 0;
    virtual bool lookup(const char* pathname, void*& data_addr, size_t& size) const = 0;

private:
    static int64_t parse_size(const std::string& str);

protected:
    /* maximum allocatable size in bytes */
    int64_t max_size;
    

}; // class Backend

} // namespace efsng

#endif /* __DATA_STORE_H__ */
