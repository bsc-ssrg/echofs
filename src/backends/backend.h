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

#include <unordered_map>

/* internal includes */
#include <logging.h>
#include <settings.h>
#include <efs-common.h>

namespace efsng {


/**
 * A pure virtual class for manipulating a backend store
 */
class backend {

public:
    class file {
public:
        virtual ~file(){}
    };

    using file_ptr = std::unique_ptr<file>;
    using buffer = std::pair<data_ptr_t, size_t>;

    struct buffer_map : public std::list<buffer> {

        buffer_map() : m_size(0) {}


        void emplace_back(data_ptr_t data, size_t size) {
            this->std::list<buffer>::emplace_back(data, size);
            m_size += size;
        }

        size_t            m_size;
    };
    
    /* iterator types */
    typedef std::unordered_map<std::string, file_ptr>::iterator iterator;
    typedef std::unordered_map<std::string, file_ptr>::const_iterator const_iterator;

    enum Type {
        UNKNOWN = -1,
        DRAM = 0,
        NVRAM_NVML,

        /* add newly registered backend types ABOVE this line */
        TOTAL_COUNT
    };

protected:
    backend() {}

public:
    virtual ~backend() {}

public:
    static Type name_to_type(const std::string& name); // probably deprecated

    static backend* builder(const std::string& type, const kv_list& backend_opts, logger& logger);
    virtual std::string name() const = 0;
    virtual uint64_t capacity() const = 0;
    virtual void load(const bfs::path& pathname) = 0;
    virtual bool exists(const char* pathname) const = 0;
    virtual void read_data(const backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const = 0;
    virtual void write_data(const backend::file& file, off_t offset, size_t size, buffer_map& bufmap) const = 0;

    virtual iterator find(const char* path) = 0;
    virtual iterator begin() = 0;
    virtual iterator end() = 0;
    virtual const_iterator cbegin() = 0;
    virtual const_iterator cend() = 0;

private:
    static int64_t parse_size(const std::string& str);
}; // class backend

} // namespace efsng

#ifdef __EFS_DEBUG__
std::ostream& operator<<(std::ostream& os, const efsng::backend::buffer& buf);
std::ostream& operator<<(std::ostream& os, const efsng::backend::buffer_map& bmap);
#endif

#endif /* __DATA_STORE_H__ */
