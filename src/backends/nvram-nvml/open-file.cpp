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

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <libpmem.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "open-file.h"
#include "../../logging.h"

namespace bfs = boost::filesystem;

namespace efsng {
namespace nvml {

fdesc::fdesc(const bfs::path& pathname, int flags)
    : m_fd(-1),
      m_pathname(pathname){

    m_fd = ::open(pathname.c_str(), flags);

    if(m_fd == -1){
        BOOST_LOG_TRIVIAL(error) << "Error loading file " << m_pathname << ": " << strerror(errno);
        throw std::runtime_error("");
    }
}

fdesc::~fdesc(){
    if(m_fd != -1 && ::close(m_fd) == -1){
        BOOST_LOG_TRIVIAL(error) << "Error closing file " << m_pathname << ": " << strerror(errno);
    }
}

size_t fdesc::get_size() const {

    struct stat stbuf;

    if(fstat(m_fd, &stbuf) == -1){
        BOOST_LOG_TRIVIAL(error) << "Error finding size of file " << m_pathname << ": " << strerror(errno);
        return 0;
    }

    return stbuf.st_size;
}

void fdesc::close() {
    if(m_fd != -1 && ::close(m_fd) == -1){
        BOOST_LOG_TRIVIAL(error) << "Error closing file " << m_pathname << ": " << strerror(errno);
        return;
    }

    m_fd = -1;
}


mapping::mapping(const bfs::path& prefix, const bfs::path& base_path, size_t min_size) {

    const bfs::path abs_path = generate_pool_path(prefix, base_path);

    BOOST_LOG_TRIVIAL(debug) << abs_path;

    /* if the mapping file already exists delete it, since we can't trust that it's the same file */
    /* TODO: we may need to change this in the future */
    if(bfs::exists(abs_path)){
        if(unlink(abs_path.c_str()) != 0){
            BOOST_LOG_TRIVIAL(error) << "Error removing file " << abs_path << ": " << strerror(errno);
            throw std::runtime_error("");
        }
    }

    void* pmem_addr;
    size_t mapping_length;

    //TODO: round mapping to a multiple of block_size

    /* create the mapping */
    if((pmem_addr = pmem_map_file(abs_path.c_str(), min_size, PMEM_FILE_CREATE | PMEM_FILE_EXCL,
                                  0666, &mapping_length, &m_is_pmem)) == NULL) {
        BOOST_LOG_TRIVIAL(error) << "Error creating pmem file " << abs_path << ": " << strerror(errno);
        throw std::runtime_error("");
    }

    /* check that the range allocated is of the required size */
    if(mapping_length != min_size){
        BOOST_LOG_TRIVIAL(error) << "File mapping of different size than requested";
        pmem_unmap(pmem_addr, mapping_length);
        throw std::runtime_error("");
    }

    m_data = pmem_addr;
    m_size = mapping_length;
}

mapping::~mapping() {
    if(pmem_unmap(m_data, m_size) == -1) {
        BOOST_LOG_TRIVIAL(error) << "Error while deleting mapping";
    }
}

void mapping::populate(const fdesc& fd) {
    m_is_pmem ? copy_data_to_pmem(fd) : copy_data_to_non_pmem(fd);
}

/** generate a name for an NVML pool using a prefix and a base_path */
bfs::path mapping::generate_pool_path(const bfs::path& prefix, const bfs::path& base_path) const {

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    bfs::path ppath(boost::uuids::to_string(uuid));

    return (base_path / prefix).string() + "-" + ppath.string();
}

ssize_t mapping::copy_data_to_pmem(const fdesc& fdesc){

    char* addr = (char*) m_data;
    char* buffer = (char*) malloc(NVML_TRANSFER_SIZE*sizeof(*buffer));

    if(buffer == NULL){
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fdesc.m_fd, buffer, NVML_TRANSFER_SIZE)) != 0){

        if(n == -1){
            if(errno != EINTR){
                BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
                return n;
            }
            /* EINTR, repeat  */
            continue;
        }

        pmem_memcpy_nodrain(addr, buffer, n);
        addr += n;
        total += n;
    }

    /* flush caches */
    pmem_drain();

    return total;
}

ssize_t mapping::copy_data_to_non_pmem(const fdesc& fdesc){

    char* addr = (char*) m_data;
    char* buffer = (char*) malloc(NVML_TRANSFER_SIZE*sizeof(*buffer));

    if(buffer == NULL){
        BOOST_LOG_TRIVIAL(error) << "Unable to allocate temporary buffer: " << strerror(errno);
    }

    ssize_t total = 0;
    ssize_t n;

    while((n = read(fdesc.m_fd, buffer, NVML_TRANSFER_SIZE)) != 0){

        if(n == -1){
            if(errno != EINTR){
                BOOST_LOG_TRIVIAL(error) << "Error while reading: " << strerror(errno);
                return n;
            }
            /* EINTR, repeat  */
            continue;
        }

        memcpy(addr, buffer, n);
        addr += n;
        total += n;
    }

    return total;
}

data_copy::data_copy()
    : m_offset(0), 
      m_data(NULL),
      m_generation(1),
      m_refs(0) {
}

data_copy::data_copy(const data_copy& orig)
    : m_offset(orig.m_offset),
      m_data(orig.m_data) {

    m_generation.store(m_generation.load());
    m_refs.store(m_refs.load());
}

snapshot::snapshot(const mapping& mapping) 
    : m_state(mapping){ }


file::file() 
    : Backend::file() {
}

file::file(const mapping& mp) 
    : efsng::Backend::file() {
    m_mappings.push_back(std::move(mp));
}

void file::add(const mapping& mp) {
    m_mappings.push_back(std::move(mp));
}

} // namespace nvml
} // namespace efsng
